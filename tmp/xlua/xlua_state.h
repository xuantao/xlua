#pragma once
#include "common.h"
#include "state.h"

XLUA_NAMESPACE_BEGIN

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

class Variant;

class Object {
    friend class internal::StateData;
    friend class State;
    Object(int ref, State* s) : ref_(ref), state_(s) {}

public:
    Object() : ref_(-1), state_(nullptr) {}
    Object(Object&& obj) { Move(obj); }
    Object(const Object& obj) : ref_(obj.ref_), state_(obj.state_) { AddRef(); }
    ~Object() { Release(); }

    void operator = (Object&& obj) {
        Release();
        Move(obj);
    }

    void operator = (const Object& obj) {
        Release();
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
    void Release();

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

private:
    inline bool IsLud() const { return IsValid() && ref_ == -1; }

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
public:
    lua_State* GetState() { return nullptr; }

    Variant LoadVar(int index) {
        auto* l = state_.l_;
        int lty = lua_type(l, index);
        size_t len = 0;
        const char* str = nullptr;
        void* ptr = nullptr;

        switch (lty) {
        case LUA_TNIL:
            return Variant();
        case LUA_TBOOLEAN:
            return Variant((bool)lua_toboolean(state_.l_, index));
        case LUA_TLIGHTUSERDATA:
            ptr = lua_touserdata(l, index);
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (internal::LightData::IsValid(ptr))
                Variant(ptr, Object(-1, this));
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            return Variant(ptr);
            break;
        case LUA_TNUMBER:
            if (lua_isinteger(l, index))
                return Variant(lua_tointeger(l, index));
            else
                return Variant(lua_tonumber(l, index));
        case LUA_TSTRING:
            str = lua_tolstring(l, index, &len);
            return Variant(str, len);
        case LUA_TTABLE:
            return Variant(VarType::kTable, Object(-1, this));
        case LUA_TFUNCTION:
            return Variant(VarType::kFunction, Object(-1, this));
        case LUA_TUSERDATA:
            ptr = lua_touserdata(l, index);
            if (internal::IsValid(static_cast<internal::UserData*>(ptr)))
                return Variant(VarType::kUserData, Object(-1, this));
            else
                return Variant();
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
            PushObject_(var.obj_);
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
                PushObject_(var.obj_);
            break;
        }
    }

    inline void PushVar(const Table& var) {
        PushObject_(var);
    }

    inline void PushVar(const Function& var) {
        PushObject_(var);
    }

    inline void PushVar(const UserData& var) {
#if XLUA_ENABLE_LUD_OPTIMIZE
        if (var.IsLud())
            lua_pushlightuserdata(state_.l_, var.ptr_);
        else
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            PushObject_(var);
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

    void PushObject_(const Object& obj) {

    }

    internal::StateData state_;
};

XLUA_NAMESPACE_END

/* */
#include "support.h"
