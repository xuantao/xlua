#pragma once
#include "core.h"
#include <assert.h>
#include <functional>

XLUA_NAMESPACE_BEGIN

/* var type
 * the type of lua var
*/
enum class VarType : int8_t {
    kNil,
    kBoolean,
    kNumber,
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
    StackGuard(lua_State* l, int off = 0) : l_(l) { Init(off); }
    StackGuard(State* s, int off = 0);
    StackGuard(StackGuard&& other) : l_(other.l_), top_(other.top_) { other.l_ = nullptr; }
    ~StackGuard() { if (l_) lua_settop(l_, top_); }

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
    CallGuard(lua_State* l, int off = 0) : StackGuard(l, off) {}
    CallGuard(State* s, int off = 0) : StackGuard(s, off) {}
    CallGuard(CallGuard&& other) : StackGuard(std::move(other)), ok_(other.ok_) {}

    CallGuard(const CallGuard&) = delete;
    void operator = (const CallGuard&) = delete;

public:
    explicit inline operator bool() const { return ok_; }

protected:
    bool ok_ = false;
};
inline bool operator == (const CallGuard& g, bool b) { return (bool)g == b; }
inline bool operator == (bool b, const CallGuard& g) { return (bool)g == b; }
inline bool operator != (const CallGuard& g, bool b) { return (bool)g != b; }
inline bool operator != (bool b, const CallGuard& g) { return (bool)g != b; }

/* process call failed boolean check */
class FailedCall : private CallGuard {
public:
    FailedCall(CallGuard&& guard) : CallGuard(std::move(guard)) {}
public:
    explicit inline operator bool() const { return false == ok_; }
};

/* easy check lua call resut and guard the lua stack */
#define XCALL_SUCC(Call) if (xlua::CallGuard guard = Call)
#define XCALL_FAIL(Call) if (xlua::FailedCall guard = Call)

/* xlua state
 * the main interface of xlua
*/
class State {
public:
    static State* Create(const char* mod);
    static State* Attach(lua_State* l, const char* mod);
    static State* GetState(lua_State* l);

    void Release();

public:
    State() = default;
    State(const State&) = delete;
    void operator = (const State&) = delete;

public:
    inline const char* GetModuleName() const { return state_.module_; }
    inline lua_State* GetLuaState() const { return state_.l_; }
    inline int GetTop() const { return lua_gettop(state_.l_); }
    inline void SetTop(int top) { lua_settop(state_.l_, top); }
    inline void PopTop(int n) { lua_pop(state_.l_, n); }
    inline bool IsNil(int index) { return lua_isnil(state_.l_, index); }
    inline void PushNil() { lua_pushnil(state_.l_); }
    inline void NewTable() { lua_newtable(state_.l_); }
    inline void Gc() { lua_gc(state_.l_, LUA_GCCOLLECT, 0); }
    inline const char* GetTypeName(int index) const { return state_.GetTypeName(index); }
    inline void* GetExtra() const { return state_.extra_; }
    inline void SetExtra(void* p) { state_.extra_ = p; }

    template <typename... Tys>
    bool IsType(int index) const {
        constexpr size_t arg_num = sizeof...(Tys);
        if (GetTop() - index + 1 < arg_num)
            return false;
        return internal::Checker<arg_num>::template Do<Tys...>(
            const_cast<State*>(this), index);
    }

    VarType GetType(int index) {
        int lty = lua_type(state_.l_, index);
        switch (lty) {
        case LUA_TNIL: return VarType::kNil;
        case LUA_TBOOLEAN: return VarType::kBoolean;
        case LUA_TLIGHTUSERDATA:
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (internal::LightUd::IsValid(lua_touserdata(state_.l_, index)))
                return VarType::kUserData;
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            return VarType::kLightUserData;
        case LUA_TNUMBER: return VarType::kNumber;
        case LUA_TSTRING: return VarType::kString;
        case LUA_TTABLE: return VarType::kTable;
        case LUA_TFUNCTION: return VarType::kFunction;
        case LUA_TUSERDATA:
            return static_cast<internal::FullUd*>(lua_touserdata(state_.l_, index))->IsValid() ?
                VarType::kUserData : VarType::kNil;
        default:
            return VarType::kNil;
        }
    }

    inline bool LoadString(const char* script, const char* chunk) {
        int ret = luaL_loadbuffer(state_.l_, script, std::char_traits<char>::length(script), chunk);
        if (LUA_OK != ret) {
            printf("load chunk faile:%s\n", lua_tostring(state_.l_, -1));
            return false;
        }
        return true;
    }

    inline bool DoString(const char* script, const char* chunk) {
        return (bool)DoString(script, chunk, std::tie());
    }

    template <typename... Rtys, typename... Args>
    inline CallGuard DoString(const char* script, const char* chunk, std::tuple<Rtys&...>&& rets, Args&&... args) {
        if (!LoadString(script, chunk))
            return CallGuard(this, -1);
        return Call(std::move(rets), std::forward<Args>(args)...);
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
    inline VarType LoadField(int index, const Ky& key) {
        int ty = lua_type(state_.l_, index);
        if (!_IS_TABLE_TYPE(ty)) {
            PushNil();
            return VarType::kNil;
        }

        index = lua_absindex(state_.l_, index);
        Push(key);
        lua_gettable(state_.l_, index);
        return GetType(-1);
    }

    template <typename Ky, typename Ty>
    inline bool SetField(int index, const Ky& key, Ty&& val) {
        int ty = lua_type(state_.l_, index);
        if (!_IS_TABLE_TYPE(ty))
            return false;

        index = lua_absindex(state_.l_, index);
        Push(key);
        Push(std::forward<Ty>(val));
        lua_settable(state_.l_, index);
        return true;
    }

    template <typename Ty, typename Ky>
    inline Ty GetField(int index, const Ky& key) {
        StackGuard guard(this);
        LoadField(index, key);
        return Get<Ty>(-1);
    }

    template <typename Ty>
    inline auto Get(int index) -> typename SupportTraits<Ty>::value_type {
        using traits = SupportTraits<Ty>;
        using supporter = typename traits::supporter;
        static_assert(traits::is_support, "not support type");
        static_assert(!traits::is_obj_value, "not support load object value directly");
        return supporter::Load(this, index);
    }

    template <typename Ty>
    inline void Push(Ty&& val) {
        using traits = SupportTraits<Ty>;
        using supporter = typename traits::supporter;
        static_assert(traits::is_support, "not support type");
        supporter::Push(this, std::forward<Ty>(val));
    }

    template <typename Fy>
    inline void PushLambda(Fy&& f) {
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
        } else {
            char stack[1024];
            GetCallStack(stack, 1024);
            printf("call failed: %s\n%s\n", lua_tostring(state_.l_, -1), stack);
        }
        return guard;
    }

    internal::StateData state_;
}; // class State

/* stack guard */
inline StackGuard::StackGuard(State* s, int off/* = 0*/)
    : l_(s->GetLuaState()) { Init(off); }

/* lua object */
class Object {
    friend class Variant;
    Object(int index, State* s) : index_(index), state_(s) {}

public:
    Object() : index_(0), state_(nullptr) {}
    Object(Object&& other) { Move(other); }
    Object(const Object& other) : index_(other.index_), state_(other.state_) { AddRef(); }
    ~Object() { DecRef(); }

public:
    inline void operator = (Object&& other) {
        DecRef();
        Move(other);
    }

    inline void operator = (const Object& other) {
        DecRef();
        index_ = other.index_;
        state_ = other.state_;
        AddRef();
    }

    inline void operator = (std::nullptr_t) {
        DecRef();
        index_ = 0;
        state_ = nullptr;
    }

    bool operator == (const Object& other) const {
        if (state_ != other.state_)
            return false;
        if (IsValid() != other.IsValid())
            return false;
        if (!IsValid())
            return true;

        StackGuard gaurd(state_);
        state_->state_.LoadRef(index_);
        state_->state_.LoadRef(other.index_);
        return lua_rawequal(state_->GetLuaState(), -2, -1) == 1;
    }

    inline bool operator != (const Object& other) const {
        return !(*this == other);
    }

    inline State* GetState() const { return state_; }
    inline bool IsValid() const { return index_ > 0 && state_ != nullptr; }

    inline void Push() const {
        assert(IsValid());
        state_->state_.LoadRef(index_);
    }

private:
    inline void Move(Object& other) {
        index_ = other.index_;
        state_ = other.state_;
        other.index_ = 0;
        other.state_ = nullptr;
    }

    inline void AddRef() {
        if (index_ > 0)
            state_->state_.AddObjRef(index_);
    }

    inline void DecRef() {
        if (index_ > 0)
            state_->state_.DecObjRef(index_);
    }

protected:
    int index_;
    State* state_;
}; // class Object

class Table;
class Function;
class UserData;

/* Variant
 * any var load from lua
*/
class Variant {
    friend class State;
    friend struct Support<Variant>;

public:
    Variant() = default;
    Variant(const Variant& other) = default;
    Variant(bool val) : type_(VarType::kBoolean), boolean_(val) {}
    Variant(int64_t val) : type_(VarType::kNumber), is_int_(true), integer_(val) {}
    Variant(double val) : type_(VarType::kNumber), number_(val) {}
    Variant(const char* val, size_t len) : type_(VarType::kString), integer_(0), str_(val, len) {}
    Variant(const Table& table);
    Variant(const Function& func);
    Variant(const UserData& userdata);

private:
    Variant(void* val) : type_(VarType::kLightUserData), ptr_(val) {}
    Variant(void* ptr, int index, State* s) : type_(VarType::kUserData), ptr_(ptr), obj_(index, s) {}
    Variant(VarType ty, int index, State* s) : type_(ty), obj_(index, s) {}

public:
    inline bool operator == (const Variant& other) const {
        if (type_ != other.type_)
            return false;

        switch (type_) {
        case xlua::VarType::kNil: return true;
        case xlua::VarType::kBoolean: return boolean_ == other.boolean_;
        case xlua::VarType::kNumber:
            if (is_int_ == other.is_int_) {
                if (is_int_) return integer_ == other.integer_;
                else return number_ == other.number_;
            } else if (is_int_) {
                return integer_ == other.ToDobule();
            } else {
                return ToDobule() == other.number_;
            }
        case xlua::VarType::kString: return str_ == other.str_;
        case xlua::VarType::kTable:
        case xlua::VarType::kFunction:
        case xlua::VarType::kUserData:
            return obj_ == other.obj_;
        case xlua::VarType::kLightUserData:
            return ptr_ == other.ptr_;
        default:
            return false;
        }
    }

    inline bool operator != (const Variant& other) const {
        return !(*this == other);
    }

    inline VarType GetType() const { return type_; }

    inline bool ToBoolean() const {
        return (type_ == VarType::kBoolean) ? boolean_ : false;
    }

    inline int64_t ToInt64() const {
        if (type_ != VarType::kNumber)
            return 0;
        else if (is_int_)
            return integer_;
        else
            return static_cast<int64_t>(number_);
    }

    inline int ToInt() const { return static_cast<int>(ToInt64()); }

    inline double ToDobule() const {
        if (type_ != VarType::kNumber)
            return 0;
        else if (is_int_)
            return static_cast<double>(integer_);
        else
            return number_;
    }

    inline float ToFloat() const { return static_cast<float>(ToDobule()); }
    inline const std::string& ToString() const {
        return (type_ == VarType::kString) ? str_ : std::string();
    }
    /* light user data */
    inline void* ToPtr() const {
        return (type_ == VarType::kLightUserData) ? ptr_ : nullptr;
    }
    /* user data */
    template <class Ty, typename std::enable_if<std::is_pointer<Ty>::value, int>::type = 0>
    inline Ty ToObj() const {
        if (type_ != VarType::kUserData)
            return nullptr;

        StackGuard guard(obj_.state_);
#if XLUA_ENABLE_LUD_OPTIMIZE
        if (obj_.index_ == 0)
            lua_pushlightuserdata(obj_.state_->GetLuaState(), ptr_);
        else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            obj_.Push();
        return obj_.state_->Get<Ty>(-1);
    }

    inline Table ToTable() const;
    inline Function ToFunction() const;
    inline UserData ToUserData() const;

private:
    bool is_int_ = false;
    VarType type_ = VarType::kNil;

    union {
        bool boolean_;
        int64_t integer_;
        double number_;
        void* ptr_;
    };

    std::string str_;
    Object obj_;
}; // class Variant

namespace internal {
    /* iterator
     * lua table iterator
    */
    template <class Oty>
    class iterator : std::iterator<std::input_iterator_tag, const std::pair<Variant, Variant>> {
    public:
        using base_type = std::iterator<std::input_iterator_tag, const std::pair<Variant, Variant>>;
        using reference = typename base_type::reference;
        using pointer = typename base_type::pointer;

        iterator() = default;
        iterator(const iterator&) = default;
        explicit iterator(const Oty& obj) : obj_(obj) {}

    public:
        inline reference operator* () const { return value_; }
        inline pointer operator -> () const { return &value_; }

        inline iterator& operator++() { MoveNext(); return *this; }
        inline iterator operator++(int) { iterator retval = *this; ++(*this); return retval; }
        inline bool operator==(const iterator& other) const { return obj_ == other.obj_ && value_.first == other.value_.first; }
        inline bool operator!=(const iterator& other) const { return !(*this == other); }

        bool MoveNext() {
            if (!obj_.IsValid())
                return false;

            auto* s = obj_.GetState();
            StackGuard guard(s);

            s->Push(obj_);
            s->Push(value_.first);
            if (lua_next(s->GetLuaState(), -2) != 0) {
                value_.second = s->Get<Variant>(-1);
                value_.first = s->Get<Variant>(-2);
            } else {
                value_.first = Variant();
            }
            return value_.first.GetType() != VarType::kNil;
        }

    private:
        Oty obj_;
        std::pair<Variant, Variant> value_;
    };
} // internal

/* lua table */
class Table : private Object {
    friend class Variant;

    Table(Object&& obj) : Object(std::move(obj)) {}
    Table(const Object& obj) : Object(obj) {}
public:
    using iterator = internal::iterator<Table>;

public:
    Table() = default;
    Table(Table&& table) : Object(std::move(table)) {}
    Table(const Table& table) : Object(table) {}

    inline void operator = (Table&& table) { Object::operator=(std::move(table)); }
    inline void operator = (const Table& table) { Object::operator=(table); }
    inline void operator = (std::nullptr_t) { Object::operator= (nullptr); }

public:
    inline State* GetState() const { return Object::GetState(); }
    inline bool IsValid() const { return Object::IsValid(); }

    inline bool operator == (const Table& table) const { return Object::operator== (table); }
    inline bool operator != (const Table& table) const { return Object::operator!= (table); }

    template <typename Ky>
    inline VarType LoadField(const Ky& key) {
        assert(IsValid());

        state_->Push(*this);
        state_->LoadField(-1, key);
        lua_remove(state_->GetLuaState(), -2);
        return state_->GetType(-1);
    }

    template <typename Ky>
    inline bool SetField(const Ky& key) {
        assert(IsValid() && state_->GetTop());

        StackGuard guard(state_, -1);
        state_->Push(*this);
        state_->Push(key);
        lua_rotate(state_->state_.l_, -3, -1);  //from:value, table, key
                                                //  to:table, key, value
        lua_settable(state_->state_.l_, -3);
        return true;
    }

    template <typename Ty, typename Ky>
    inline Ty GetField(const Ky& key) {
        assert(IsValid());

        StackGuard guard(state_);
        state_->Push(*this);
        state_->LoadField(-1, key);
        return state_->Get<Ty>(-1);
    }

    template <typename Ky, typename Ty>
    inline bool SetField(const Ky& key, Ty&& val) {
        assert(IsValid());

        StackGuard guard(state_);
        state_->Push(*this);
        state_->SetField(-1, key, std::forward<Ty>(val));
        return true;
    }

    template <typename Ky, typename... Rys, typename... Args>
    inline CallGuard Call(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args) {
        assert(IsValid());

        LoadField(key);
        if (state_->GetType(-1) == VarType::kFunction)
            return state_->Call(std::move(ret), *this, std::forward<Args>(args)...);

        state_->PopTop(1);
        return CallGuard();
    }

    template <typename Ky, typename... Rys, typename... Args>
    inline CallGuard DotCall(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args) {
        assert(IsValid());

        LoadField(key);
        if (state_->GetType(-1) == VarType::kFunction)
            return state_->Call(std::move(ret), std::forward<Args>(args)...);

        state_->PopTop(1);
        return CallGuard();
    }

public:
    inline iterator begin() const {
        iterator it(*this);
        it.MoveNext();
        return it;
    }

    inline iterator end() const {
        return iterator(*this);
    }
}; // class Table

/* lua user data */
class UserData : private Object {
    friend class Variant;
    friend class State;

    UserData(void* ptr, Object&& obj) : Object(std::move(obj)), ptr_(ptr) {}
    UserData(void* ptr, const Object& obj) : Object(obj), ptr_(ptr) {}

public:
    UserData() : ptr_(nullptr) {}
    UserData(UserData&& ud) : Object(std::move(ud)), ptr_(ud.ptr_) { ud.ptr_ = nullptr; }
    UserData(const UserData& ud) : Object(ud), ptr_(ud.ptr_) {}

    inline void operator = (UserData&& ud) {
        Object::operator=(std::move(ud));
        ptr_ = ud.ptr_;
        ud.ptr_ = nullptr;
    }

    inline void operator = (const UserData& ud) {
        Object::operator=(ud);
        ptr_ = ud.ptr_;
    }

    inline void operator = (std::nullptr_t) {
        Object::operator= (nullptr);
        ptr_ = nullptr;
    }

public:
    inline State* GetState() const { return Object::GetState(); }
    inline bool IsValid() const { return state_ != nullptr && ptr_ != nullptr; }

    inline bool operator == (const UserData& ud) const {
        return state_ == ud.state_ && ptr_ == ud.ptr_;
    }
    inline bool operator != (const UserData& ud) const {
        return !(*this == ud);
    }

    template <typename Ty>
    inline bool IsType() const {
        if (!IsValid())
            return false;

        StackGuard guard(state_);
        state_->Push(*this);
        return state_->IsType<Ty>(-1);
    }

    template <typename Ty>
    inline Ty As() const {
        assert(IsValid());
        /* if the type is shared_ptr, only can be load by supporter */
        StackGuard guard(state_);
        state_->Push(*this);
        return state_->Get<Ty>(-1);
    }

    template <typename Ky>
    inline VarType LoadField(const Ky& key) {
        assert(IsValid());

        state_->Push(*this);
        state_->LoadField(-1, key);
        lua_remove(state_->GetLuaState(), -2);
        return state_->GetType(-1);
    }

    template <typename Ky>
    inline bool SetField(const Ky& key) {
        assert(IsValid() && state_->GetTop());

        StackGuard guard(state_, -1);
        state_->Push(*this);
        state_->Push(key);
        lua_rotate(state_->state_.l_, -3, -1);  //from:value, table, key
                                                //  to:table, key, value
        lua_settable(state_->state_.l_, -3);
        return true;
    }

    template <typename Ty, typename Ky>
    inline Ty GetField(const Ky& key) {
        assert(IsValid());

        StackGuard guard(state_);
        state_->Push(*this);
        state_->LoadField(-1, key);
        return state_->Get<Ty>(-1);
    }

    template <typename Ky, typename Ty>
    inline bool SetField(const Ky& key, Ty&& val) {
        assert(IsValid());

        StackGuard guard(state_);
        state_->Push(*this);
        state_->SetField(-1, key, std::forward<Ty>(val));
        return true;
    }

    template <typename Ky, typename... Rys, typename... Args>
    inline CallGuard Call(const Ky& key, std::tuple<Rys&...>&& ret, Args&&... args) {
        assert(IsValid());

        LoadField(key);
        if (state_->GetType(-1) == VarType::kFunction)
            return state_->Call(std::move(ret), *this, std::forward<Args>(args)...);

        state_->PopTop(1);
        return CallGuard();
    }

    inline void* ToPtr() const {
        if (!IsValid())
            return nullptr;

#if XLUA_ENABLE_LUD_OPTIMIZE
        if (IsLud())
            return internal::UnpackLightUd(ToLud());
#endif // XLUA_ENABLE_LUD_OPTIMIZE

        auto* ud = static_cast<internal::FullUd*>(ptr_);
        if (ud->major != internal::UdMajor::kDeclaredType || ud->minor != internal::UdMinor::kPtr)
            return ud->ptr;
        if (ud->desc->weak_index)
            return ud->desc->weak_proc.getter(ud->ref);
        return ud->ptr;
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    inline bool IsLud() const { return IsValid() && index_ == 0; }
    inline internal::LightUd ToLud() const { return internal::LightUd::Make(ptr_); }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

private:
    void* ptr_;
}; // class UserData

/* lua function */
class Function : private Object {
    friend class Variant;
    friend class State;

    Function(Object&& obj) : Object(std::move(obj)) {}
    Function(const Object& obj) : Object(obj) {}

public:
    Function() = default;
    Function(Function&& func) : Object(std::move(func)) {}
    Function(const Function& func) : Object(func) {}

    inline void operator = (Function&& func) { Object::operator=(std::move(func)); }
    inline void operator = (const Function& func) { Object::operator=(func); }
    inline void operator = (std::nullptr_t) { Object::operator= (nullptr); }

public:
    inline State* GetState() const { return Object::GetState(); }
    inline bool IsValid() const { return Object::IsValid(); }
    inline bool operator == (const Function& func) const { return Object::operator== (func); }
    inline bool operator != (const Function& func) const { return Object::operator!= (func); }

public:
    template <typename... Rys, typename... Args>
    inline CallGuard operator ()(std::tuple<Rys&...>&& ret, Args&&... args) const {
        return Call(std::move(ret), std::forward<Args>(args)...);
    }

    template <typename... Rys, typename... Args>
    inline CallGuard Call(std::tuple<Rys&...>&& ret, Args&&... args) const {
        assert(IsValid());

        state_->Push(*this);
        return state_->Call(std::move(ret), std::forward<Args>(args)...);
    }
}; // class Function

/* variant detail */
inline Variant::Variant(const Table& table)
    : type_(table.IsValid() ? VarType::kTable : VarType::kNil), obj_(table) {}
inline Variant::Variant(const Function& func)
    : type_(func.IsValid() ? VarType::kFunction : VarType::kNil), obj_(func) {}
inline Variant::Variant(const UserData& userdata)
    : type_(userdata.IsValid() ? VarType::kUserData : VarType::kNil), obj_(userdata), ptr_(userdata.ptr_) {}

inline Table Variant::ToTable() const {
    if (type_ == VarType::kTable)
        return Table(obj_);
    return Table();
}

inline Function Variant::ToFunction() const {
    if (type_ == VarType::kFunction)
        return Function(obj_);
    return Function();
}

inline UserData Variant::ToUserData() const {
    if (type_ == VarType::kUserData)
        return UserData(ptr_, obj_);
    return UserData();
}

/* state holder
 * support lua coroutine
 * if call from lua coroutine, the lua_State is different from the main thread
 * this object will push/pop the activing lua_State, make sure the operations with the right lua stack
*/
class Holder {
public:
    Holder() = default;
    Holder(Holder&& h) : l_(h.l_), s_(h.s_) { h.s_ = nullptr; }
    ~Holder() { if (s_) s_->state_.l_ = l_; }

    Holder(const Holder& h) = delete;
    void operator = (const Holder&) = delete;

    inline void operator = (Holder&& h) {
        l_ = h.l_;
        s_ = h.s_;
        h.s_ = nullptr;
    }

    inline State* Get() const { return s_; }
    inline State* operator->() const { return s_; }
    inline operator State*() const { return s_; }

public:
    static Holder Load(lua_State* l) {
        Holder h;
        h.s_ = State::GetState(l);
        if (h.s_) {
            h.l_ = h.s_->state_.l_;
            h.s_->state_.l_ = l;
        }
        return h;
    }

private:
    lua_State* l_ = nullptr;
    State* s_ = nullptr;
};

XLUA_NAMESPACE_END

/* include the basic support implementation */
#include "support.hpp"
