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
    StackGuard(State* l);
    StackGuard(State* l, int off);
    StackGuard(StackGuard&& other) : l_(other.l_), top_(other.top_) {
        other.l_ = nullptr;
    }

    ~StackGuard();

    StackGuard(const StackGuard&) = delete;
    StackGuard& operator = (const StackGuard&) = delete;

protected:
    int top_;
    State* l_;
};

/* lua函数调用堆栈保护
 * lua函数执行以后返回值存放在栈上, 如果清空栈可能会触发GC
 * 导致引用的局部变量失效造成未定义行为
*/
class CallGuard : private StackGuard {
    friend class State;

public:
    CallGuard() : StackGuard() {}
    CallGuard(State* l) : StackGuard(l) {}
    CallGuard(State* l, int off) : StackGuard(l, off) {}
    CallGuard(CallGuard&& other) : StackGuard(std::move(other)), ok_(other.ok_) {}

    CallGuard(const CallGuard&) = delete;
    void operator = (const CallGuard&) = delete;

public:
    explicit operator bool() const { return ok_; }

private:
    bool ok_ = false;
};

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
    const char* GetTypeName(int index) const { return state_.GetTypeName(index); }
    inline lua_State* GetLuaState() const { return state_.l_; }
    inline int GetTop() const { return lua_gettop(state_.l_); }
    inline void SetTop(int top) { lua_settop(state_.l_, top); }
    inline void PopTop(int n) { lua_pop(state_.l_, n); }
    inline bool DoString(const char* script, const char* chunk) {
        return state_.DoString(script, chunk);
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

    Variant LoadVar(int index) {
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
            lua_pushnil(l);
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
                lua_pushnil(state_.l_);
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
                lua_pushnil(state_.l_);
            }
            break;
        }
    }

    inline void PushVar(const Table& var) {
        if (var.IsValid()) {
            assert(this == var.state_);
            state_.LoadRef(var.index_);
        } else {
            lua_pushnil(state_.l_);
        }
    }

    inline void PushVar(const Function& var) {
        if (var.IsValid()) {
            assert(this == var.state_);
            state_.LoadRef(var.index_);
        } else {
            lua_pushnil(state_.l_);
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
            lua_pushnil(state_.l_);
        }
    }

    inline bool IsNil(int index) {
        return lua_isnil(state_.l_, index);
    }

    inline void PushNil() {
        lua_pushnil(state_.l_);
    }

    inline void NewTable() {
        lua_newtable(state_.l_);
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
        return Load<Ty>(-1);
    }

    inline bool SetGlobal(const char* path, bool create) {
        return state_.SetGlobal(path, create, false);
    }

    template <typename Ty>
    bool SetGlobal(const char* path, Ty& val, bool create) {
        StackGuard guard(this);
        Push(val);
        return SetGlobal(path, create);
    }

    template <typename Ky>
    VarType LoadField(int index, const Ky& key) {
        if (!lua_istable(state_.l_, index)) {
            PushNil();
            return VarType.kNil;
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
        PushVar(val);
        lua_settable(state_.l_, index);
        return true;
    }

    template <typename Ky, typename Ty>
    Ty GetField(int index, const Ky& key) {
        StackGuard guard(this);
        LoadField(index, key);
        return Load<Ty>(-1);
    }

    template <typename... Tys>
    bool IsType(int index) const {
        constexpr size_t arg_num = sizeof...(Tys);
        if (GetTop() - index + 1 < arg_num)
            return false;
        return internal::Checker<arg_num>::template Do<Tys...>(
            const_cast<State*>(this), index);
    }

    template <typename Ty, typename std::enable_if<std::is_pointer<Ty>::value, int>::type = 0>
    Ty Load(int index) {
        static_assert(IsSupport<Ty>::value, "not support type");
        return Support<typename PurifyType<Ty>::type>::Load(this, index);
    }

    template <typename Ty, typename std::enable_if<!std::is_pointer<Ty>::value, int>::type = 0>
    Ty Load(int index) {
        //static_assert(!IsObjectType<Ty>::value, "can not directly load object type, use load pointer instead");
        return Support<typename PurifyType<Ty>::type>::Load(this, index);
    }

    template <typename Ty>
    void Push(Ty&& val) {
        static_assert(IsSupport<Ty>::value, "not support push value to lua");
        Support<typename PurifyType<Ty>::type>::Push(this, std::forward<Ty>(val));
    }

    template <typename... Ty>
    inline void LoadMul(int index, std::tuple<Ty&...>&& ret) {
        LoadMul(index, std::move(ret), make_index_sequence_t<sizeof...(Ty)>());
    }

    template <typename... Ty, size_t... Idxs>
    inline void LoadMul(int index, std::tuple<Ty&...>&& ret, index_sequence<Idxs...>) {
        using ints = int[];
        (void)ints {
            0, (std::get<Idxs>(ret) = Load<Ty>(index++), 0)...
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
    inline CallGuard Call(std::tuple<Rys&...>&& ret, Args&&... args) {
        int top = GetTop();
        CallGuard guard(this, -1);

        PushMul(std::forward<Args>(args)...);
        if (lua_pcall(state_.l_, sizeof...(Args), sizeof...(Rys), 0) == LUA_OK) {
            LoadMul(top, std::move(ret));
            guard.ok_ = true;
        } else {
            //TODO: log error meesage
        }
        return std::move(guard);
    }

    internal::StateData state_;
};

/* lua stack guarder */
inline StackGuard::StackGuard(State* l) : l_(l) {
    top_ = l->GetTop();
}

inline StackGuard::StackGuard(State* l, int off) : l_(l) {
    top_ = l_->GetTop() + off;
}

inline StackGuard::~StackGuard() {
    if (l_) l_->SetTop(top_);
}

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
    return state_->Load<Ty>(-1);
}

/* lua table */
template <typename Ky>
VarType Table::LoadField(const Ky& key) {
    if (!IsValid())
        return VarType::kNil;

    StackGuard guard(state_);
    state_->PushVar(*this);
    state_->LoadField(key, -1);
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

    return state_->Load<Ty>(-1);
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

    lua_pop(state_->state_.l_, 1);
    return CallGuard();
}

template <typename Ky, typename... Rys, typename... Args>
CallGuard Table::DotCall(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args) {
    if (!IsValid())
        return CallGuard();

    LoadField(key);
    if (state_->GetType(-1) == VarType::kFunction)
        return state_->Call(std::move(ret), std::forward<Args>(args)...);

    lua_pop(state_->state_.l_, 1);
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
