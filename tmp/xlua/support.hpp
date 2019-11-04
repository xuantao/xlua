#pragma once
XLUA_NAMESPACE_BEGIN

template <typename Cat, typename Ty, typename Tag = Ty>
struct SupportCategory {
    typedef Cat category;
    typedef Ty value_type;
    typedef Tag tag_type;

    static const size_t tag;
};
// assign static const member
template <typename Cat, typename Ty, typename Tag >
const size_t SupportCategory<Cat, Ty, Tag>::tag = typeid(Tag).hash_code();

template <typename Ty, typename Tag = Ty>
struct ValueCategory : SupportCategory<value_category_tag, Ty, Tag> {};

template <typename Ty, typename Tag = Ty>
struct DeclaredCategory : SupportCategory<declared_category_tag, Ty, Tag> {};

template <typename Ty, typename Tag = Ty>
struct CollectionCategory : SupportCategory<collection_category_tag, Ty, Tag> {};

//template <typename Sy, typename Ty, typename Tag>
//struct SmartPtrCategory : value_category_tag<Sy, Tag> {
//    typedef Ty ptr_type;
//};

namespace internal {
    template <typename Ty>
    struct IsLarge {
        static constexpr bool is_integer = std::numeric_limits<Ty>::is_integer;
        static constexpr bool value = std::numeric_limits<Ty>::digits > 32;
    };

    struct void_tag {};
    struct enum_tag {};
    struct number_tag {};
    struct declared_tag {};

    /* traits dispacth tag, declared/enum/void tag */
    template <typename Ty>
    struct DisaptchTag {
        typedef typename std::conditional<IsLuaType<Ty>::value, declared_tag,
            typename std::conditional<std::is_enum<Ty>::value, enum_tag, void_tag>::type>::type type_tag;
    };

    template <typename Ty>
    struct DisaptchTag<Ty*> {
        typedef typename std::conditional<IsLuaType<Ty>::value, declared_tag, void_tag>::type type_tag;
    };

    template <typename Ty, typename Tag>
    struct ExportSupport : SupportCategory<not_support_tag, void_tag> {
    };

    /* declared type pointer */
    template <typename Ty>
    struct ExportSupport<Ty*, declared_tag> : DeclaredCategory<Ty> {
        static inline const TypeDesc* Desc() { return xLuaGetTypeDesc(Identity<Ty>()); }
        static inline const char* Name() { return Desc()->name;  }
        static inline bool Check(State* l, int index) {
            return lua_isnil(l->GetLuaState(), index) || l->state_.IsUd<Ty>(index);
        }
        static inline Ty* Load(State* l, int index) {
            return l->state_.LoadUd<Ty>(index);
        }
        static inline void Push(State* l, Ty* ptr) {
            l->state_.PushUd(ptr);
        }
    };

    /* declared type value */
    template <typename Ty>
    struct ExportSupport<Ty, declared_tag> : DeclaredCategory<Ty> {
        typedef ExportSupport<Ty*, declared_tag> supporter;

        static inline const TypeDesc* Desc() { return supporter::Desc(); }
        static inline const char* Name() { return supporter::Name(); }
        static inline bool Check(State* l, int index) {
            return supporter::Load(l, index) != nullptr;
        }
        static inline ObjectWrapper<Ty> Load(State* l, int index) {
            return ObjectWrapper<Ty>(supporter::Load(l, index));
        }
        static inline void Push(State* l, const Ty& obj) {
            l->state_.PushUd(obj);
        }
        static inline void Push(State* l, Ty&& obj) {
            l->state_.PushUd(std::move(obj));
        }
    };

    /* enum support */
    template <typename Ty>
    struct ExportSupport<Ty, enum_tag> : ValueCategory<typename std::underlying_type<Ty>::type, number_tag> {
        typedef Support<typename std::underlying_type<Ty>::type> supporter;
        typedef typename supporter::value_type underlying_type;
        typedef Ty value_type;

        static inline const char* Name() { return typeid(Ty).name(); }
        static inline bool Check(State* l, int index) {
            return supporter::Check(l, index);
        }
        static inline value_type Load(State* l, int index) {
            return static_cast<value_type>(supporter::Load(l, index));
        }
        static inline void Push(State* l, Ty value) {
            supporter::Push(l, static_cast<underlying_type>(value));
        }
    };

    /* number value support */
    template <typename Ty>
    struct NumberSupport : ValueCategory<Ty, number_tag> {
        static inline bool Check(State* l, int index) {
            return lua_type(l->GetLuaState(), index) == LUA_TNUMBER;
        }

        static inline Ty Load(State* l, int index) {
            return DoLoad<Ty>(l, index);
        }

        static inline void Push(State* l, Ty value) {
            DoPush(l, value);
        }

    private:
        template <typename U, typename std::enable_if<std::is_floating_point<U>::value, int>::type = 0>
        static inline void DoPush(State* l, U val) {
            lua_pushnumber(l->GetLuaState(), static_cast<U>(val));
        }

        template <typename U, typename std::enable_if<IsLarge<U>::is_integer && !IsLarge<U>::value, int>::type = 0>
        static inline void DoPush(State* l, U val) {
            lua_pushnumber(l->GetLuaState(), val);
        }

        template <typename U, typename std::enable_if<IsLarge<U>::is_integer && IsLarge<U>::value, int>::type = 0>
        static inline void DoPush(State* l, U val) {
            lua_pushinteger(l->GetLuaState(), val);
        }

        template <typename U, typename std::enable_if<std::is_floating_point<U>::value, int>::type = 0>
        static inline U DoLoad(State* l, int index) {
            return static_cast<U>(lua_tonumber(l->GetLuaState(), index));
        }

        template <typename U, typename std::enable_if<IsLarge<U>::is_integer && !IsLarge<U>::value, int>::type = 0>
        static inline U DoLoad(State* l, int index) {
            return static_cast<U>(lua_tonumber(l->GetLuaState(), index));
        }

        template <typename U, typename std::enable_if<IsLarge<U>::is_integer && IsLarge<U>::value, int>::type = 0>
        static inline U DoLoad(State* l, int index) {
            return static_cast<U>(lua_tointeger(l->GetLuaState(), index));
        }
    };
} // namespace internal

/* export type support */
template <typename Ty>
struct Support : internal::ExportSupport<Ty, typename internal::DisaptchTag<Ty>::type_tag> {
};

/* lua var support */
template <>
struct Support<Variant> : ValueCategory<Variant> {
    static inline const char* Name() { return "xlua::Variant"; }
    static inline bool Check(State* l, int index) { return true; }
    static inline Variant Load(State* l, int index) { return l->LoadVar(index); }
    static inline void Push(State* l, const Variant& var) { l->PushVar(var); }
};

template <>
struct Support<Table> : ValueCategory<Table> {
    static inline const char* Name() { return "xlua::Table"; }
    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TTABLE;
    }
    static inline Table Load(State* l, int index) { return l->LoadVar(index).ToTable(); }
    static inline void Push(State* l, const Table& var) { l->PushVar(var); }
};

template <>
struct Support<Function> : ValueCategory<Function> {
    static inline const char* Name() { return "xlua::Function"; }
    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TFUNCTION;
    }
    static inline Function Load(State* l, int index) { return l->LoadVar(index).ToFunction(); }
    static inline void Push(State* l, const Function& var) { l->PushVar(var); }
};

template <>
struct Support<UserData> : ValueCategory<UserData> {
    static inline const char* Name() { return "xlua::UserVar"; }
    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TUSERDATA || lty == LUA_TLIGHTUSERDATA;
    }
    static inline UserData Load(State* l, int index) { return l->LoadVar(index).ToUserData(); }
    static inline void Push(State* l, const UserData& var) { l->PushVar(var); }
};

/* nil support */
template <>
struct Support<std::nullptr_t> : ValueCategory<std::nullptr_t> {
    static inline bool Check(State*, int) { return false; }
    static inline std::nullptr_t Load(State*, int) = delete;
    static inline void Push(State* l, std::nullptr_t) { lua_pushnil(l->GetLuaState()); }
};

#define _XLUA_NUMBER_SUPPORT(Type)                                  \
template <> struct Support<Type> : internal::NumberSupport<Type> {  \
    static const char* Name() { return #Type; }                     \
};

_XLUA_NUMBER_SUPPORT(char)
_XLUA_NUMBER_SUPPORT(unsigned char)
_XLUA_NUMBER_SUPPORT(short)
_XLUA_NUMBER_SUPPORT(unsigned short)
//_XLUA_NUMBER_SUPPORT(unsigned)
_XLUA_NUMBER_SUPPORT(int)
_XLUA_NUMBER_SUPPORT(unsigned int)
_XLUA_NUMBER_SUPPORT(long)
_XLUA_NUMBER_SUPPORT(unsigned long)
_XLUA_NUMBER_SUPPORT(long long)
_XLUA_NUMBER_SUPPORT(unsigned long long)
_XLUA_NUMBER_SUPPORT(float)
_XLUA_NUMBER_SUPPORT(double)

/* boolean type support */
struct boolean_tag {};

template <>
struct Support<bool> : ValueCategory<bool, boolean_tag> {
    static inline const char* Name() { return "boolean"; }

    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TBOOLEAN;
    }

    static inline bool Load(State* l, int index) {
        return lua_toboolean(l->GetLuaState(), index);
    }

    static inline void Push(State* l, bool b) {
        lua_pushboolean(l->GetLuaState(), b);
    }
};

struct string_tag {};

/* string type support */
template <>
struct Support<const char*> : ValueCategory<const char*, string_tag> {
    static inline const char* Name() { return "const char*"; }

    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TSTRING;
    }

    static inline const char* Load(State* l, int index) {
        return lua_tostring(l->GetLuaState(), index);
    }

    static inline void Push(State* l, const char* p) {
        if (p) lua_pushstring(l->GetLuaState(), p);
        else lua_pushnil(l->GetLuaState());
    }
};

template <>
struct Support<char*> : ValueCategory<char*, string_tag> {
    static inline const char* Name() {
        return "char*";
    }

    static inline bool Check(State* l, int index) {
        return Support<const char*>::Check(l, index);
    }

    static inline char* Load(State* l, int index) = delete;

    static inline void Push(State* l, char* p) {
        Support<const char*>::Push(l, p);
    }
};

// char array is act as const char*
template <size_t N>
struct Support<char[N]> : ValueCategory<char*, string_tag> {
    static_assert(N > 0, "char array size must greater than 0");
    static inline const char* Name() {
        return "char[]";
    }

    static inline bool Check(State* l, int index) {
        return Support<const char*>::Check(l, index);
    }

    static inline char* Load(State* l, int index) = delete;

    static inline void Push(State* l, const char* p) {
        Support<const char*>::Push(l, p);
    }
};

template <size_t N>
struct Support<const char[N]> : ValueCategory<const char*, string_tag> {
    static_assert(N > 0, "char array size must greater than 0");
    static inline const char* Name() {
        return "const char[]";
    }

    static inline bool Check(State* l, int index) {
        return Support<const char*>::Check(l, index);
    }

    static inline char* Load(State* l, int index) = delete;

    static inline void Push(State* l, const char* p) {
        Support<const char*>::Push(l, p);
    }
};

template <class Trait, class Alloc>
struct Support<std::basic_string<char, Trait, Alloc>> : ValueCategory<std::basic_string<char, Trait, Alloc>, string_tag>{
    typedef std::basic_string<char, Trait, Alloc> value_type;

    static inline const char* Name() {
        return "std::string";
    }

    static inline bool Check(State* l, int index) {
        return Support<const char*>::Check(l, index);
    }

    static inline value_type Load(State* l, int index) {
        size_t len = 0;
        const char* s = lua_tolstring(l->GetLuaState(), index, &len);
        if (s && len)
            return value_type(s, len);
        return value_type();
    }

    static inline void Push(State* l, const value_type& str) {
        lua_pushlstring(l->GetLuaState(), str.c_str(), str.size());
    }
};

/* light user data support */
template <>
struct Support<void*> : ValueCategory<void*> {
    static inline const char* Name() {
        return "void*";
    }

    static inline bool Check(State* l, int index) {
        auto lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TLIGHTUSERDATA;
    }

    static inline void* Load(State* l, int index) {
        return lua_touserdata(l->GetLuaState(), index);
    }

    static inline void Push(State* l, void* p) {
        if (p) lua_pushlightuserdata(l->GetLuaState(), p);
        else lua_pushnil(l->GetLuaState());
    }
};

template <>
struct Support<const void*> : ValueCategory<void*> {
    static inline const char* Name() {
        return "const void*";
    }

    static inline bool Check(State* l, int index) {
        auto lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TLIGHTUSERDATA;
    }

    static inline const void* Load(State* l, int index) {
        return lua_touserdata(l->GetLuaState(), index);
    }

    static inline void Push(State* l, const void* p) = delete;
};

namespace internal {
    template <typename Fy, typename Ry, typename... Args, size_t... Idxs>
    inline auto DoCall(State* s, Fy f, index_sequence<Idxs...> idxs) -> typename std::enable_if<!std::is_void<Ry>::value, int>::type {
        if (s->IsType<Args...>(1)) {
            //l->Push(f(param::LoadParam<typename param::ParamType<Args>::type>(l, Idxs + 1)...));
            return 1;
        } else {
            //TODO: log error
            return 0;
        }
    }

    template <typename Fy, typename Ry, typename... Args, size_t... Idxs>
    inline auto DoCall(State* s, Fy f, index_sequence<Idxs...> idxs) -> typename std::enable_if<std::is_void<Ry>::value, int>::type {
        if (s->IsType<Args...>(1)) {
            //l->Push(f(param::LoadParam<typename param::ParamType<Args>::type>(l, Idxs + 1)...));
        } else {
            //TODO: log error
        }
        return 0;
    }

    template <typename Fy, typename Ry, typename... Args>
    inline int DoCall(State* s, Fy f) {
        return DoCall<Fy, Ry, Args...>(s, f, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ry, typename... Args>
    auto MakeFunc(Function&& f) -> typename std::enable_if<std::is_void<Ry>::value, std::function<Ry(Args...)>>::type{
        return [=](Args... args) {
            f(std::tie(), args...);
        };
    }

    template <typename Ry, typename... Args>
    auto MakeFunc(Function&& f) -> typename std::enable_if<!std::is_void<Ry>::value, std::function<Ry(Args...)>>::type {
        return [=](Args... args) -> Ry {
            Ry val;
            f(std::tie(val), args...);
            return std::move(val);
        };
    }
} // namespace internal
/* function support */
template <>
struct Support<int(lua_State*)> : ValueCategory<int(lua_State*)> {
    typedef int (*value_type)(lua_State*);

    static inline const char* Name() { return "cfunction"; }

    static inline bool Check(State* l, int index) {
        return lua_isnil(l->GetLuaState(), index) ||
            lua_iscfunction(l->GetLuaState(), index);
    }

    static inline void Push(State* l, value_type f) {
        if (f) lua_pushcfunction(l->GetLuaState(), f);
        else lua_pushnil(l->GetLuaState());
    }

    static inline value_type Load(State* l, int index) {
        return lua_tocfunction(l->GetLuaState(), index);
    }
};

template <>
struct Support<int(State*)> : ValueCategory<int(State*)>{
    typedef int (*value_type)(State*);

    static inline const char* Name() { return "cfunction"; }

    static inline bool Check(State* l, int index) {
        return lua_isnil(l->GetLuaState(), index) ||
            lua_tocfunction(l->GetLuaState(), index) == &Call;
    }

    static inline value_type Load(State* l, int index) {
        if (lua_tocfunction(l->GetLuaState(), index) != &Call)
            return nullptr;

        lua_getupvalue(l->GetLuaState(), index, 1);
        void* f = lua_touserdata(l->GetLuaState(), -1);
        lua_pop(l->GetLuaState(), 1);
        return static_cast<value_type>(f);
    }

    static inline void Push(State* l, value_type f) {
        if (!f) {
            lua_pushnil(l->GetLuaState());
        } else {
            lua_pushlightuserdata(l->GetLuaState(), static_cast<void*>(f));
            lua_pushcclosure(l->GetLuaState(), &Call, 1);
        }
    }

private:
    static int Call(lua_State* l) {
        auto* f = static_cast<value_type>(lua_touserdata(l, lua_upvalueindex(1)));
        return f(internal::GetState(l));
    };
};

template <typename Ry, typename... Args>
struct Support<Ry(Args...)> : ValueCategory<Ry(Args...)> {
    typedef Ry (*value_type)(Args...);
    typedef value_category_tag category;

    static inline bool Name() { return "xfunction"; }

    static inline bool Check(State* l, int index) {
        return lua_isnil(l->GetLuaState(), index) ||
            lua_tocfunction(l->GetLuaState(), index) == &Call;
    }

    static inline value_type Load(State* l, int index) {
        if (lua_tocfunction(l->GetLuaState(), index) != &Call)
            return nullptr;

        lua_getupvalue(l->GetLuaState(), index, 1);
        void* f = lua_touserdata(l->GetLuaState(), -1);
        lua_pop(l->GetLuaState(), 1);
        return static_cast<value_type>(f);
    }

    static inline void Push(State* l, value_type f) {
        if (!f) {
            lua_pushnil(l->GetLuaState());
        } else {
            lua_pushlightuserdata(l->GetLuaState(), static_cast<void*>(f));
            lua_pushcclosure(l->GetLuaState(), &Call, 1);
        }
    }

private:
    int Call(lua_State* l) {
        auto f = static_cast<value_type>(lua_touserdata(l, lua_upvalueindex(1)));
        return internal::DoCall<value_type, Ry, Args...>(internal::GetState(l), f);
    }
};

template <typename Ry, typename... Args>
struct Support<std::function<Ry(Args...)>> : ValueCategory<std::function<Ry(Args...)>> {
    typedef std::function<Ry(Args...)> value_type;
    typedef internal::ObjData<value_type> ObjData;
    static inline const char* Name() { return "std::function"; }

    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TFUNCTION;
    }

    static value_type Load(State* l, int index) {
        int lty = lua_type(l->GetLuaState(), index);
        if (lty == LUA_TNIL || lty != LUA_TFUNCTION)
            return value_type();
        // same type
        if (lua_tocfunction(l->GetLuaState(), index) == &Call) {
            lua_getupvalue(l->GetLuaState(), index, 1);
            auto* d = static_cast<ObjData*>(lua_touserdata(l->GetLuaState(), -1));
            lua_pop(l->GetLuaState(), 1);
            return d->obj;
        }

        // unkonwn real type funtion
        auto f = l->Load<Function>(index);
        return internal::MakeFunc<Ry, Args...>(std::move(f));
    }

    static inline void Push(State* l, const value_type& val) {
        if (!val) {
            lua_pushnil(l->GetLuaState());
        } else {
            l->state_.NewAloneObj<value_type>(val);
            lua_pushcclosure(l->GetLuaState(), &Call, 1);
        }
    }

private:
    static int Call(lua_State* l) {
        auto* d = static_cast<ObjData*>(lua_touserdata(l, lua_upvalueindex(1)));
        return internal::DoCall<value_type&, Ry, Args...>(internal::GetState(l), d->obj);
    }
};

struct std_shared_ptr_tag {};

/* smart ptr support */
template <typename Ty>
struct Support<std::shared_ptr<Ty>> : ValueCategory<std::shared_ptr<Ty>, std_shared_ptr_tag> {
    static_assert(std::is_base_of<object_category_tag_, typename Support<Ty>::category>::value, "only support object");
    typedef ValueCategory<std::shared_ptr<Ty>, std_shared_ptr_tag> base_type_;
    typedef typename base_type_::value_type value_type;

    static const char* Name() { return "std::shared_ptr"; }

    static bool Check(State* l, int index) {
        if (lua_isnil(l->GetLuaState(), index))
            return true;

        auto* ud = l->state_.LoadRawUd(index);
        if (ud == nullptr || ud->minor != internal::UdMinor::SmartPtr)
            return false;

        auto* ptr = static_cast<internal::ObjUd*>(ud)->As<internal::SmartPtrData>();
        if (ptr->tag != base_type_::tag)
            return false;

        return l->state_.IsUd(ud, Support<Ty>::Desc());
    }

    static value_type Load(State* l, int index) {
        if (lua_isnil(l->GetLuaState(), index))
            return value_type();

        auto* ud = l->state_.LoadRawUd(index);
        if (ud == nullptr || ud->minor != internal::UdMinor::kSmartPtr)
            return value_type();

        auto* ptr = static_cast<internal::ObjUd*>(ud)->As<internal::SmartPtrData>();
        if (ptr->tag != base_type_::tag)
            return value_type();

        return value_type(*static_cast<value_type*>(ptr->data),
            (Ty*)_XLUA_TO_SUPER_PTR(ud->ptr, ud->desc, Support<Ty>::Desc()));
    }

    static void Push(State* l, const value_type& ptr) {
        if (!ptr)
            lua_pushnil(l->GetLuaState());
        else
            l->state_.PushSmartPtr(ptr.get(), ptr, base_type_::tag);
    }

    static void Push(State* l, value_type&& ptr) {
        if (!ptr)
            lua_pushnil(l->GetLuaState());
        else
            l->state_.PushSmartPtr(ptr.get(), std::move(ptr), base_type_::tag);
    }
};

namespace internal {
    template <typename Ty>
    struct UniqueObj {
        inline Ty& Instance() {
            static Ty obj;
            return obj;
        }
    };

    template <typename Ty>
    struct VectorCol final : ICollection {
        typedef std::vector<Ty> vector;
        typedef Ty value_type;

        int Index(void* obj, State* l) override {
            return 0;
        }

        int NewIndex(void* obj, State* l) override {
            return 0;
        }

        int Insert(void* obj, State* l) override {
            return 0;
        }

        int Remove(void* obj, State* l) override {
            return 0;
        }

        int Length(void* obj) override {
            return 0;
        }

        void Clear(void* obj) override {

        }
    };

    template <typename Cy, typename Pc>
    struct CollectionSupport : CollectionCategory<Cy> {
        typedef Cy value_type;
        typedef collection_category_tag category;

        static inline ICollection* Desc() { return &UniqueObj<Pc>::Instance(); }

        static inline bool Check(State* l, int index) {
            return Support<Cy*>::Load(l, index) != nullptr;
        }

        static inline ObjectWrapper<Cy> Load(State* l, int index) {
            return ObjectWrapper<Cy>(Support<Cy*>::Load(l, index));
        }

        static inline void Push(State* l, const value_type& obj) {
            l->state_.PushUd(obj);
        }
    };

    template <typename Cy, typename Pc>
    struct CollectionSupport<Cy*, Pc> : CollectionCategory<Cy> {
        typedef Cy* value_type;
        typedef collection_category_tag category;

        static inline ICollection* Desc() { return &UniqueObj<Pc>::Instance(); }

        static inline bool Check(State* l, int index) {
            return lua_isnil(l->GetLuaState(), index) || l->state_.IsUd<Cy>(index);
        }

        static inline value_type Load(State* l, int index) {
            return l->state_.LoadUd<Cy>(index);
        }

        static inline void Push(State* l, value_type ptr) {
            l->state_.PushUd(ptr);
        }
    };
}

/* vector support */
#define _COLLECTION_SUPPORT(Cy, Py)                                     \
template <typename Ty>                                                  \
struct Support<Cy<Ty>> : internal::CollectionSupport<Cy<Ty>, Py<Ty>> {  \
    static inline const char* Name() { return #Cy; }                    \
};                                                                      \
template <typename Ty>                                                  \
struct Support<Cy<Ty>*> : internal::CollectionSupport<Cy<Ty>*, Py<Ty>> {\
    static inline const char* Name() { return #Cy "*"; }                \
};

_COLLECTION_SUPPORT(std::vector, internal::VectorCol)

XLUA_NAMESPACE_END
