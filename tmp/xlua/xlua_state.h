#pragma once
#include "common.h"
#include "state.h"

XLUA_NAMESPACE_BEGIN

class Variant;

class Object {
    friend class internal::StateData;
    friend class State;
    Object(int ref, State* s) : ref_(ref), state_(s) {}

public:
    Object() : ref_(-1), state_(nullptr) {}
    Object(Object&& obj) { Move(obj); }
    Object(const Object& obj) : ref_(obj.ref_), state_(obj.state_) { AddRef(); }
    ~Object() { DecRef(); }

    void operator = (Object&& obj) {
        DecRef();
        Move(obj);
    }

    void operator = (const Object& obj) {
        DecRef();
        ref_ = obj.ref_;
        state_ = obj.state_;
        AddRef();
    }

public:
    inline State* GetState() const { return state_; }
    inline bool IsValid() const { return ref_ != -1 && state_ != nullptr; }

private:
    void Move(Object& obj) {
        ref_ = obj.ref_;
        state_ = obj.state_;
        obj.ref_ = -1;
        obj.state_ = nullptr;
    }

    void AddRef();
    void DecRef();

protected:
    int ref_;
    State* state_;
};

class Table : public Object {
    friend class Variant;
    friend class State;

    Table(Object&& obj) : Object(std::move(obj)) {}
    Table(const Object& obj) : Object(obj) {}

public:
    Table() { }

public:
    //TODO:

};

class Function : public Object {
    friend class Variant;
    friend class State;

    Function(Object&& obj) : Object(std::move(obj)) {}
    Function(const Object& obj) : Object(obj) {}

public:
    Function() {}

public:
    //TODO: call
};

class UserData : public Object {
    friend class Variant;
    friend class State;

    UserData(void* ptr, Object&& obj) : Object(std::move(obj)), ptr_(ptr) {}
    UserData(void* ptr, const Object& obj) : Object(obj), ptr_(ptr) {}

public:
    UserData() : ptr_(nullptr) {}

public:
    inline bool IsValid() const { return state_ != nullptr; }

    template <typename Ty>
    inline bool IsType() {
        using purify_type = typename PurifyType<Ty>::type;
        static_assert(std::is_base_of<ObjectCategory_, Support<purify_type>::category,
            "not support type");
        if (!IsValid())
            return false;

#if XLUA_ENABLE_LUD_OPTIMIZE
        if (IsLud())
            return internal::IsUd(Make(ptr_), Support<purify_type>::Desc());
        else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            return internal::IsUd(static_cast<internal::UserData*>(ptr_), Support<purify_type>::Desc());
    }

    template <typename Ty, typename std::enable_if<std::is_reference<Ty>::value, int>::type = 0>
    inline Ty As() {
        using purify_type = typename PurifyType<Ty>::type;
        static_assert(std::is_base_of<ObjectCategory_, Support<purify_type>::category,
            "not support type");
        if (!IsValid())
            return nullptr;

#if XLUA_ENABLE_LUD_OPTIMIZE
        if (IsLud())
            return internal::As<purify_type>(Make(ptr_));
        else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            return internal::As<purify_type>(static_cast<internal::UserData*>(ptr_));
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
private:
    inline bool IsLud() const { return IsValid() && ref_ == -1; }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

private:
    void* ptr_;
};

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
    lua_State* GetState() { return nullptr; }

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
            if (internal::LightData::IsValid(ptr))
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
            if (internal::IsValid(static_cast<internal::UserData*>(ptr)))
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
            if (internal::LightData::IsValid(ptr))
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
            if (internal::IsValid(static_cast<internal::UserData*>(ptr)))
                return Variant(VarType::kUserData, Object(state_.RefObj(index), this));
            break;
        }
        return Variant();
    }

    void PushVar(const Variant& var) {
        auto* l = state_.l_;
        switch (var.GetType())
        {
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
            PushObj_(var.obj_);
            break;
        case VarType::kLightUserData:
            lua_pushlightuserdata(l, var.ptr_);
            break;
        case VarType::kUserData:
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (var.obj_.ref_ == -1)
                lua_pushlightuserdata(l, var.ptr_);
            else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
                PushObj_(var.obj_);
            break;
        }
    }

    inline void PushVar(const Table& var) {
        PushObj_(var);
    }

    inline void PushVar(const Function& var) {
        PushObj_(var);
    }

    inline void PushVar(const UserData& var) {
#if XLUA_ENABLE_LUD_OPTIMIZE
        if (var.IsLud())
            lua_pushlightuserdata(state_.l_, var.ptr_);
        else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            PushObj_(var);
    }

    template <typename Ty>
    bool IsType(int index) const {
        static_assert(IsSupport<Ty>::value, "not support type");
        return Support<typename PurifyType<Ty>::type;>::Check(this, index);
    }

    template <typename Ty, typename std::enable_if<std::is_pointer<Ty>::value, int>::type = 0>
    Ty Load(int index) {
        static_assert(IsSupport<Ty>::value, "not support type");
        return Support<typename PurifyType<Ty>::type>::Load(this, index);
    }

    template <typename Ty, typename std::enable_if<!std::is_pointer<Ty>::value, int>::type = 0>
    Ty Load(int index) {
        static_assert(IsSupport<Ty>::value, "not support type");
        static_assert(!std::is_reference<Ty>::value || std::is_const<Ty>::value,
            "not support load reference value");
        return Support<typename PurifyType<Ty>::type>::Load(this, index);
    }

    template <typename Ty>
    void Push(const Ty& val) {
        static_assert(IsSupport<Ty>::value, "not support push value to lua");
        Support<typename PurifyType<Ty>::type>::Push(this, val);
    }

    void PushObj_(const Object& obj) {
        if (!obj.IsValid())
            return;
        state_.LoadRef(obj.ref_);
    }

    internal::StateData state_;
};

inline void Object::AddRef() {
    if (ref_ != -1)
        state_->state_.AddRef(ref_);
}

inline void Object::DecRef() {
    if (ref_ != -1)
        state_->state_.DecRef(ref_);
}

XLUA_NAMESPACE_END

/* */
#include "support.h"
