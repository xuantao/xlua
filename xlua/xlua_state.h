#pragma once
#include "state.h"
#include <assert.h>
#include <functional>

XLUA_NAMESPACE_BEGIN

class Variant;

enum class VarType : int8_t {
    kNil,
    kBoolean,
    kNumber,
    kInteger,
    kString,
    kTable,
    kFunction,
    kLightUserData,
    kUserData,
};

/* lua stack guarder */
class StackGuard {
public:
    StackGuard() : top_(0), l_(nullptr) {}
    StackGuard(std::nullptr_t) : top_(0), l_(nullptr) {}
    StackGuard(lua_State* l) : l_(l) { Init(0); }
    StackGuard(lua_State* l, int off) : l_(l) { Init(0); }
    StackGuard(State* s);
    StackGuard(State* s, int off);
    StackGuard(StackGuard&& other) : l_(other.l_), top_(other.top_) { other.l_ = nullptr; }

    ~StackGuard() {
        if (l_) lua_settop(l_, top_);
    }

    StackGuard(const StackGuard&) = delete;
    StackGuard& operator = (const StackGuard&) = delete;

private:
    inline void Init(int off) {
        if (l_) top_ = lua_gettop(l_) + off;
    }

protected:
    int top_;
    lua_State* l_;
};

/* lua函数调用堆栈保护
 * lua函数执行以后返回值存放在栈上, 如果清空栈可能会触发GC
 * 导致引用的局部变量失效造成未定义行为
*/
class CallGuard : private StackGuard {
    friend class State;

public:
    CallGuard() : StackGuard() {}
    CallGuard(std::nullptr_t) : StackGuard() {}
    CallGuard(lua_State* l) : StackGuard(l) {}
    CallGuard(lua_State* l, int off) : StackGuard(l, off) {}
    CallGuard(State* s) : StackGuard(s) {}
    CallGuard(State* s, int off) : StackGuard(s, off) {}
    CallGuard(CallGuard&& other) : StackGuard(std::move(other)), ok_(other.ok_) {}

    CallGuard(const CallGuard&) = delete;
    void operator = (const CallGuard&) = delete;

public:
    explicit inline operator bool() const { return ok_; }

private:
    bool ok_ = false;
};

inline bool operator == (const CallGuard& g, bool ok) { return (bool)g == ok; }
inline bool operator == (bool ok, const CallGuard& g) { return ok == (bool)g; }
inline bool operator != (const CallGuard& g, bool ok) { return (bool)g != ok; }
inline bool operator != (bool ok, const CallGuard& g) { return ok != (bool)g; }

/* process call failed boolean check */
class FailedCall : private CallGuard {
public:
    FailedCall(CallGuard&& guard) : CallGuard(std::move(guard)) {}

public:
    explicit inline operator bool() const{
        return false == (bool)(*static_cast<const CallGuard*>(this));
    }
};

/* easy check lua call resut and guard the lua stack */
#define XLUA_IF_CALL_SUCC(Call) if (xlua::CallGuard guard = Call)
#define XLUA_IF_CALL_FAIL(Call) if (xlua::FailedCall guard = Call)

/* lua object */
class Object {
    friend class State;
    Object(int index, State* s) : index_(index), state_(s) {}

public:
    Object() : index_(0), state_(nullptr) {}
    Object(Object&& obj) { Move(obj); }
    Object(const Object& obj) : index_(obj.index_), state_(obj.state_) { AddRef(); }
    ~Object() { DecRef(); }

    void operator = (Object&& obj) {
        DecRef();
        Move(obj);
    }

    void operator = (const Object& obj) {
        DecRef();
        index_ = obj.index_;
        state_ = obj.state_;
        AddRef();
    }

public:
    inline State* GetState() const { return state_; }
    inline bool IsValid() const { return index_ > 0 && state_ != nullptr; }

private:
    void Move(Object& obj) {
        index_ = obj.index_;
        state_ = obj.state_;
        obj.index_ = 0;
        obj.state_ = nullptr;
    }

    void AddRef();
    void DecRef();

protected:
    int index_;
    State* state_;
};

/* lua table */
class Table : public Object {
    friend class Variant;
    friend class State;

    Table(Object&& obj) : Object(std::move(obj)) {}
    Table(const Object& obj) : Object(obj) {}

public:
    Table() {}

public:
    template <typename Ky>
    VarType LoadField(const Ky& key);

    template <typename Ky>
    void SetField(const Ky& key);

    template <typename Ky, typename Ty>
    Ty GetField(const Ky& key);

    template <typename Ky, typename Ty>
    void SetField(const Ky& key, Ty& val);

    template <typename Ky, typename... Rys, typename... Args>
    CallGuard Call(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args);

    template <typename Ky, typename... Rys, typename... Args>
    CallGuard DotCall(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args);

    //TODO: iterator
};

class Function : public Object {
    friend class Variant;
    friend class State;

    Function(Object&& obj) : Object(std::move(obj)) {}
    Function(const Object& obj) : Object(obj) {}

public:
    Function() {}

public:
    template <typename... Rys, typename... Args>
    inline CallGuard operator ()(std::tuple<Rys&...>&& ret, Args&&... args) const {
        return Call(std::move(ret), std::forward<Args>(args)...);
    }

    template <typename... Rys, typename... Args>
    CallGuard Call(std::tuple<Rys&...>&& ret, Args&&... args) const;
};

class UserData : public Object {
    friend class Variant;
    friend class State;

    UserData(void* ptr, Object&& obj) : Object(std::move(obj)), ptr_(ptr) {}
    UserData(void* ptr, const Object& obj) : Object(obj), ptr_(ptr) {}

public:
    UserData() : ptr_(nullptr) {}

public:
#if XLUA_ENABLE_LUD_OPTIMIZE
    inline bool IsValid() const { return state_ != nullptr; }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    template <typename Ty>
    inline bool IsType();

    template <typename Ty>
    inline Ty As();

#if XLUA_ENABLE_LUD_OPTIMIZE
private:
    inline bool IsLud() const { return IsValid() && index_ == -1; }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

private:
    void* ptr_;
};

class Variant {
    friend class State;
public:
    Variant() : type_(VarType::kNil) {}
    Variant(bool val) : type_(VarType::kBoolean), boolean_(val) {}
    Variant(int64_t val) : type_(VarType::kInteger), integer_(val) {}
    Variant(double val) : type_(VarType::kNumber), number_(val) {}
    Variant(const char* val, size_t len) : type_(VarType::kString), str_(val, len) {}
    Variant(void* val) : type_(VarType::kLightUserData), ptr_(val) {}
    Variant(void* ptr, const Object& obj) : type_(VarType::kUserData), ptr_(ptr), obj_(obj) {}
    Variant(VarType ty, const Object& obj) : type_(ty), obj_(obj) {}
    Variant(const Table& table) : type_(VarType::kTable), obj_(table) {}
    Variant(const Function& func) : type_(VarType::kFunction), obj_(func) {}
    Variant(const UserData& ud) : type_(VarType::kUserData), ptr_(ud.ptr_), obj_(ud) {}

public:
    inline VarType GetType() const { return type_; }

    inline bool ToBoolean() const {
        if (type_ == VarType::kBoolean)
            return boolean_;
        return false;
    }

    inline int64_t ToInt64() const {
        if (type_ == VarType::kNumber)
            return static_cast<int64_t>(number_);
        else if (type_ == VarType::kInteger)
            return static_cast<int64_t>(integer_);
        return 0;
    }

    inline int ToInt() const {
        return static_cast<int>(ToInt64());
    }

    inline double ToDobule() const {
        if (type_ == VarType::kNumber)
            return number_;
        else if (type_ == VarType::kInteger)
            return static_cast<double>(integer_);
        return 0;
    }

    inline float ToFloat() const {
        return static_cast<float>(ToDobule());
    }

    inline const std::string& ToString() const {
        if (type_ == VarType::kString)
            return str_;
        return sEmptyString;
    }

    inline void* ToPtr() const {
        if (type_ == VarType::kLightUserData || type_ == VarType::kUserData)
            return ptr_;
        return nullptr;
    }

    inline Table ToTable() const {
        if (type_ == VarType::kTable)
            return Table(obj_);
        return Table();
    }

    inline Function ToFunction() const {
        if (type_ == VarType::kFunction)
            return Function(obj_);
        return Function();
    }

    inline UserData ToUserData() const {
        if (type_ == VarType::kUserData)
            return UserData(ptr_, obj_);
        return UserData();
    }

private:
    VarType type_;

    union {
        bool boolean_;
        int64_t integer_;
        double number_;
        void* ptr_;
    };

    std::string str_;
    Object obj_;
    static std::string sEmptyString;
};

class State {
    friend class Object;
public:
    State() = default;
    State(const State&) = delete;
    void operator = (const State&) = delete;

public:
    inline void Release() { internal::Destory(this); }

public:
    const char* GetTypeName(int index) const { return state_.GetTypeName(index); }
    inline lua_State* GetLuaState() const { return state_.l_; }
    inline int GetTop() const { return lua_gettop(state_.l_); }
    inline void SetTop(int top) { lua_settop(state_.l_, top); }
    inline void PopTop(int n) { lua_pop(state_.l_, n); }
    inline bool IsNil(int index) { return state_.IsNil(index); }
    inline void PushNil() { state_.PushNil(); }
    inline void NewTable() { state_.NewTable(); }
    inline void Gc() { lua_gc(state_.l_, LUA_GCCOLLECT, 0); }

    template <typename... Tys>
    bool IsType(int index) const {
        constexpr size_t arg_num = sizeof...(Tys);
        if (GetTop() - index + 1 < arg_num)
            return false;
        return internal::Checker<arg_num>::template Do<Tys...>(
            const_cast<State*>(this), index);
    }

    template <typename... Args>
    CallGuard DoString(const char* script, const char* chunk, Args&&... args) {
        CallGuard guard(this);
        int ret = luaL_loadbuffer(state_.l_, script, std::char_traits<char>::length(script), chunk);
        if (LUA_OK != ret)
            return guard;

        PushMul(std::forward<Args>(args)...);
        ret = lua_pcall(state_.l_, (int)sizeof...(Args), LUA_MULTRET, 0);
        if (LUA_OK != ret)
            return guard;

        guard.ok_ = true;
        return guard;
    }

    inline size_t GetCallStack(char* buff, size_t size) {
        return state_.GetCallStack(buff, size);
    }

    /* load global var on stack */
    inline VarType LoadGlobal(const char* path) {
        state_.LoadGlobal(path);
        return GetType(-1);
    }

    template <typename Ty>
    inline Ty GetGlobal(const char* path) {
        StackGuard guard(this);
        LoadGlobal(path);
        return Get<Ty>(-1);
    }

    /* set the stack top value as global value and pop stack */
    inline bool SetGlobal(const char* path) {
        return state_.SetGlobal(path, false, false);
    }

    template <typename Ty>
    inline bool SetGlobal(const char* path, Ty&& val) {
        StackGuard guard(this);
        Push(std::forward<Ty>(val));
        return SetGlobal(path);
    }

    /* set global value, is the path is not exist will auto create the table */
    inline bool MakeGlobal(const char* path) {
        return state_.SetGlobal(path, true, false);
    }

    template <typename Ty>
    inline bool MakeGlobal(const char* path, Ty&& val) {
        StackGuard guard(this);
        Push(std::forward<Ty>(val));
        return MakeGlobal(path);
    }

    template <typename Ky>
    VarType LoadField(int index, const Ky& key) {
        if (!lua_istable(state_.l_, index)) {
            PushNil();
            return VarType::kNil;
        }

        index = lua_absindex(state_.l_, index);
        Push(key);
        lua_gettable(state_.l_, index);
        return GetType(-1);
    }

    template <typename Ky, typename Ty>
    bool SetField(int index, const Ky& key, Ty& val) {
        if (!lua_istable(state_.l_, index))
            return false;

        index = lua_absindex(state_.l_, index);
        Push(key);
        Push(val);
        lua_settable(state_.l_, index);
        return true;
    }

    template <typename Ty, typename Ky>
    Ty GetField(int index, const Ky& key) {
        StackGuard guard(this);
        LoadField(index, key);
        return Get<Ty>(-1);
    }

    template <typename Ty>
    auto Get(int index) -> typename SupportTraits<Ty>::value_type {
        using traits = SupportTraits<Ty>;
        using supporter = typename traits::supporter;
        static_assert(traits::is_support, "not support type");
        static_assert(!traits::is_obj_value, "not support load object value directly");
        return supporter::Load(this, index);
    }

    template <typename Ty>
    void Push(Ty&& val) {
        using traits = SupportTraits<Ty>;
        using supporter = typename traits::supporter;
        static_assert(traits::is_support, "not support type");
        supporter::Push(this, std::forward<Ty>(val));
    }

    template <typename Fy>
    void PushLambda(Fy&& f) {
        using type = typename PurifyType<Fy>::type;
        Support<internal::Lambda<type>>::Push(this, internal::Lambda<type>(std::forward<Fy>(f)));
    }

    template <typename... Ty>
    inline void GetMul(int index, std::tuple<Ty&...>&& ret) {
        GetMul(index, std::move(ret), make_index_sequence_t<sizeof...(Ty)>());
    }

    template <typename... Ty, size_t... Idxs>
    inline void GetMul(int index, std::tuple<Ty&...>&& ret, index_sequence<Idxs...>) {
        using ints = int[];
        (void)ints {
            0, (std::get<Idxs>(ret) = Get<Ty>(index++), 0)...
        };
    }

    template<typename... Ty>
    inline void PushMul(Ty&&... vals) {
        using ints = int[];
        (void)ints {
            0, (Push(std::forward<Ty>(vals)), 0)...
        };
    }

    template <typename... Rys, typename... Args>
    inline CallGuard Call(const char* global, std::tuple<Rys&...>&& ret, Args&&... args) {
        //if (LoadGlobal(global) != VarType::kFunction) {
        //    PopTop(1);
        //    return CallGuard();
        //}
        LoadGlobal(global);
        return Call(std::move(ret), std::forward<Args>(args)...);
    }

    template <typename... Rys, typename... Args>
    inline CallGuard Call(std::tuple<Rys&...>&& ret, Args&&... args) {
        int top = GetTop(); // do call function is on the top stack
        CallGuard guard(this, -1);

        PushMul(std::forward<Args>(args)...);
        if (lua_pcall(state_.l_, sizeof...(Args), sizeof...(Rys), 0) == LUA_OK) {
            GetMul(top, std::move(ret));
            guard.ok_ = true;
        }
        return guard;
    }

    VarType GetType(int index) {
        int lty = lua_type(state_.l_, index);
        void* ptr = nullptr;
        VarType vt = VarType::kNil;

        switch (lty) {
            case LUA_TNIL:
                break;
            case LUA_TBOOLEAN:
                vt = VarType::kBoolean;
                break;
            case LUA_TLIGHTUSERDATA:
                ptr = lua_touserdata(state_.l_, index);
#if XLUA_ENABLE_LUD_OPTIMIZE
                if (internal::LightUd::IsValid(ptr))
                    vt = VarType::kUserData;
                else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
                    vt = VarType::kLightUserData;
                break;
            case LUA_TNUMBER:
                if (lua_isinteger(state_.l_, index))
                    vt = VarType::kInteger;
                else
                    vt = VarType::kNumber;
                break;
            case LUA_TSTRING:
                vt = VarType::kString;
                break;
            case LUA_TTABLE:
                vt = VarType::kTable;
                break;
            case LUA_TFUNCTION:
                vt = VarType::kFunction;
                break;
            case LUA_TUSERDATA:
                ptr = lua_touserdata(state_.l_, index);
                if (static_cast<internal::FullUd*>(ptr)->IsValid())
                    vt = VarType::kUserData;
                break;
        }
        return vt;
    }

    Variant GetVar(int index) {
        auto* l = state_.l_;
        int lty = lua_type(l, index);
        size_t len = 0;
        const char* str = nullptr;
        void* ptr = nullptr;

        switch (lty) {
            case LUA_TNIL:
                break;
            case LUA_TBOOLEAN:
                return Variant((bool)lua_toboolean(state_.l_, index));
            case LUA_TLIGHTUSERDATA:
                ptr = lua_touserdata(l, index);
#if XLUA_ENABLE_LUD_OPTIMIZE
                if (internal::LightUd::IsValid(ptr))
                    return Variant(ptr, Object(-1, this));
#endif // XLUA_ENABLE_LUD_OPTIMIZE
                return Variant(ptr);
            case LUA_TNUMBER:
                if (lua_isinteger(l, index))
                    return Variant(lua_tointeger(l, index));
                else
                    return Variant(lua_tonumber(l, index));
            case LUA_TSTRING:
                str = lua_tolstring(l, index, &len);
                return Variant(str, len);
            case LUA_TTABLE:
                return Variant(VarType::kTable, Object(state_.RefObj(index), this));
            case LUA_TFUNCTION:
                return Variant(VarType::kFunction, Object(state_.RefObj(index), this));
            case LUA_TUSERDATA:
                ptr = lua_touserdata(l, index);
                if (static_cast<internal::FullUd*>(ptr)->IsValid())
                    return Variant(VarType::kUserData, Object(state_.RefObj(index), this));
                break;
        }
        return Variant();
    }

    void PushVar(const Variant& var) {
        auto* l = state_.l_;
        switch (var.GetType()) {
        case VarType::kNil:
            state_.PushNil();
            break;
        case VarType::kBoolean:
            lua_pushboolean(l, var.ToBoolean());
            break;
        case VarType::kNumber:
            lua_pushnumber(l, var.ToDobule());
            break;
        case VarType::kInteger:
            lua_pushinteger(l, var.ToInt64());
            break;
        case VarType::kString:
            lua_pushlstring(l, var.str_.c_str(), var.str_.length());
            break;
        case VarType::kTable:
        case VarType::kFunction:
            if (var.obj_.IsValid()) {
                assert(this == var.obj_.state_);
                state_.LoadRef(var.obj_.index_);
            } else {
                state_.PushNil();
            }
            break;
        case VarType::kLightUserData:
            lua_pushlightuserdata(l, var.ptr_);
            break;
        case VarType::kUserData:
            if (var.obj_.IsValid()) {
                assert(this == var.obj_.state_);
#if XLUA_ENABLE_LUD_OPTIMIZE
                if (var.obj_.index_ == -1)
                    lua_pushlightuserdata(l, var.ptr_);
                else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
                    state_.LoadRef(var.obj_.index_);
            } else {
                state_.PushNil();
            }
            break;
        }
    }

    inline void PushVar(const Table& var) {
        if (var.IsValid()) {
            assert(this == var.state_);
            state_.LoadRef(var.index_);
        } else {
            state_.PushNil();
        }
    }

    inline void PushVar(const Function& var) {
        if (var.IsValid()) {
            assert(this == var.state_);
            state_.LoadRef(var.index_);
        } else {
            state_.PushNil();
        }
    }

    inline void PushVar(const UserData& var) {
        if (var.IsValid()) {
            assert(this == var.state_);
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (var.IsLud())
                lua_pushlightuserdata(state_.l_, var.ptr_);
            else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
                state_.LoadRef(var.index_);
        } else {
            state_.PushNil();
        }
    }

    internal::StateData state_;
};

/* stack guard */
inline StackGuard::StackGuard(State* s) : l_(s->GetLuaState()) { Init(0); }
inline StackGuard::StackGuard(State* s, int off) : l_(s->GetLuaState()) { Init(off); }

/* lua object */
inline void Object::AddRef() {
    if (index_ > 0)
        state_->state_.AddObjRef(index_);
}

inline void Object::DecRef() {
    if (index_ > 0)
        state_->state_.DecObjRef(index_);
}

/* lua user data */
template <typename Ty>
inline bool UserData::IsType() {
    if (!IsValid())
        return false;

    StackGuard guard(state_);
    state_->PushVar(*this);
    return state_->IsType<Ty>(-1);
}

template <typename Ty>
inline Ty UserData::As() {
    StackGuard guard(state_);
    if (!IsValid())
        state_->PushNil();
    else
        state_->PushVar(*this);
    return state_->Get<Ty>(-1);
}

/* lua table */
template <typename Ky>
VarType Table::LoadField(const Ky& key) {
    if (!IsValid())
        return VarType::kNil;

    StackGuard guard(state_);
    state_->PushVar(*this);
    state_->LoadField(-1, key);
    lua_remove(state_->GetLuaState(), -2);
    return state_->GetType(-1);
}

template <typename Ky>
void Table::SetField(const Ky& key) {
    if (!IsValid() || state_->GetTop() == 0)
        return;

    StackGuard guard(state_);
    state_->PushVar(*this);
    state_->Push(key);
    lua_rotate(state_->state_.l_, -3, 1);   //TODO: 这里还不确定参数是否合法
    lua_settable(state_->state_.l_, -3);
}

template <typename Ky, typename Ty>
Ty Table::GetField(const Ky& key) {
    StackGuard guard(state_);
    if (!IsValid()) {
        state_->PushNil();
    } else {
        state_->PushVar(*this);
        state_->LoadField(-1, key);
    }

    return state_->Get<Ty>(-1);
}

template <typename Ky, typename Ty>
void Table::SetField(const Ky& key, Ty& val) {
    if (!IsValid())
        return;

    StackGuard guard(state_);
    state_->PushVar(*this);
    state_->SetField(-1, key, val);
}

template <typename Ky, typename... Rys, typename... Args>
CallGuard Table::Call(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args) {
    if (!IsValid())
        return CallGuard();

    LoadField(key);
    if (state_->GetType(-1) == VarType::kFunction)
        return state_->Call(std::move(ret), *this, std::forward<Args>(args)...);

    state_->PopTop(1);
    return CallGuard();
}

template <typename Ky, typename... Rys, typename... Args>
CallGuard Table::DotCall(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args) {
    if (!IsValid())
        return CallGuard();

    LoadField(key);
    if (state_->GetType(-1) == VarType::kFunction)
        return state_->Call(std::move(ret), std::forward<Args>(args)...);

    state_->PopTop(1);
    return CallGuard();
}

/* lua function */
template <typename... Rys, typename... Args>
inline CallGuard Function::Call(std::tuple<Rys&...>&& ret, Args&&... args) const {
    if (!IsValid())
        return CallGuard();

    state_->PushVar(*this);
    return state_->Call(std::move(ret), std::forward<Args>(args)...);
}

XLUA_NAMESPACE_END

/* include the basic support implementation */
#include "support.hpp"
