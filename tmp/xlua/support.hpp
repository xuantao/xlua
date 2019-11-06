#pragma once
XLUA_NAMESPACE_BEGIN

template <typename Cat, typename Ty, bool AllowNil, typename Tag = Ty>
struct SupportCategory {
    typedef Cat category;   // value, collection, delcarted type
    typedef Ty value_type;
    typedef Tag tag_type;

    static constexpr bool allow_nil = AllowNil; // load as function parameter is allow nil
    static const size_t tag;                    // tag id, specify a group type
};
// assign static const member
template <typename Cat, typename Ty, bool AllowNil, typename Tag>
const size_t SupportCategory<Cat, Ty, AllowNil, Tag>::tag = typeid(Tag).hash_code();

template <typename Ty, bool AllowNil, typename Tag = Ty>
struct ValueCategory : SupportCategory<value_category_tag, Ty, AllowNil, Tag> {};

template <typename Ty, typename Tag = Ty>
struct DeclaredCategory : SupportCategory<declared_category_tag, Ty, std::is_pointer<Ty>::value, Tag> {};

template <typename Ty, typename Tag = Ty>
struct CollectionCategory : SupportCategory<collection_category_tag, Ty, std::is_pointer<Ty>::value, Tag> {};

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
    struct ExportSupport : SupportCategory<not_support_tag, void, true, void_tag> {
    };

    /* declared type pointer */
    template <typename Ty>
    struct ExportSupport<Ty*, declared_tag> : DeclaredCategory<Ty> {
        static inline const TypeDesc* Desc() { return xLuaGetTypeDesc(Identity<Ty>()); }
        static inline const char* Name() { return Desc()->name;  }
        static inline bool Check(State* l, int index) {
            return l->state_.IsUd<Ty>(index);
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
    struct ExportSupport<Ty, enum_tag> : ValueCategory<typename std::underlying_type<Ty>::type, false, number_tag> {
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
    struct NumberSupport : ValueCategory<Ty, false, number_tag> {
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
struct Support<Variant> : ValueCategory<Variant, true> {
    static inline const char* Name() { return "xlua::Variant"; }
    static inline bool Check(State* s, int index) { return true; }
    static inline Variant Load(State* s, int index) { return s->LoadVar(index); }
    static inline void Push(State* s, const Variant& var) { s->PushVar(var); }
};

template <>
struct Support<Table> : ValueCategory<Table, true> {
    static inline const char* Name() { return "xlua::Table"; }
    static inline bool Check(State* s, int index) {
        return lua_type(s->GetLuaState(), index) == LUA_TTABLE;
    }
    static inline Table Load(State* s, int index) { return s->LoadVar(index).ToTable(); }
    static inline void Push(State* s, const Table& var) { s->PushVar(var); }
};

template <>
struct Support<Function> : ValueCategory<Function, true> {
    static inline const char* Name() { return "xlua::Function"; }
    static inline bool Check(State* s, int index) {
        return lua_type(s->GetLuaState(), index) == LUA_TFUNCTION;
    }
    static inline Function Load(State* s, int index) { return s->LoadVar(index).ToFunction(); }
    static inline void Push(State* s, const Function& var) { s->PushVar(var); }
};

template <>
struct Support<UserData> : ValueCategory<UserData, true> {
    static inline const char* Name() { return "xlua::UserData"; }
    static inline bool Check(State* s, int index) {
        int lty = lua_type(s->GetLuaState(), index);
        return lty == LUA_TUSERDATA || lty == LUA_TLIGHTUSERDATA;
    }
    static inline UserData Load(State* s, int index) { return s->LoadVar(index).ToUserData(); }
    static inline void Push(State* s, const UserData& var) { s->PushVar(var); }
};

/* nil support */
template <>
struct Support<std::nullptr_t> : ValueCategory<std::nullptr_t, true> {
    static inline bool Check(State* s, int index) { return lua_isnil(s->GetLuaState(), index); }
    static inline std::nullptr_t Load(State*, int) { return nullptr; }
    static inline void Push(State* s, std::nullptr_t) { lua_pushnil(s->GetLuaState()); }
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
struct Support<bool> : ValueCategory<bool, true, boolean_tag> {
    static inline const char* Name() { return "boolean"; }

    static inline bool Check(State* s, int index) {
        int lty = lua_type(s->GetLuaState(), index);
        return lty == LUA_TNIL || lty == LUA_TBOOLEAN;
    }

    static inline bool Load(State* s, int index) {
        return lua_toboolean(s->GetLuaState(), index);
    }

    static inline void Push(State* s, bool b) {
        lua_pushboolean(s->GetLuaState(), b);
    }
};

/* string type support */
struct string_tag {};

template <>
struct Support<const char*> : ValueCategory<const char*, true, string_tag> {
    static inline const char* Name() { return "const char*"; }

    static inline bool Check(State* s, int index) {
        return lua_type(s->GetLuaState(), index) == LUA_TSTRING;
    }

    static inline const char* Load(State* s, int index) {
        return lua_tostring(s->GetLuaState(), index);
    }

    static inline void Push(State* s, const char* p) {
        if (p) lua_pushstring(s->GetLuaState(), p);
        else lua_pushnil(s->GetLuaState());
    }
};

template <>
struct Support<char*> : ValueCategory<char*, true, string_tag> {
    static inline const char* Name() {
        return "char*";
    }

    static inline bool Check(State* s, int index) {
        return Support<const char*>::Check(s, index);
    }

    static inline char* Load(State* s, int index) = delete;

    static inline void Push(State* s, char* p) {
        Support<const char*>::Push(s, p);
    }
};

// char array is act as const char*
template <size_t N>
struct Support<char[N]> : ValueCategory<char*, true, string_tag> {
    static_assert(N > 0, "char array size must greater than 0");
    static inline const char* Name() {
        return "char[]";
    }

    static inline bool Check(State* s, int index) {
        return Support<const char*>::Check(s, index);
    }

    static inline char* Load(State* s, int index) = delete;

    static inline void Push(State* s, const char* p) {
        Support<const char*>::Push(s, p);
    }
};

template <size_t N>
struct Support<const char[N]> : ValueCategory<const char*, true, string_tag> {
    static_assert(N > 0, "char array size must greater than 0");
    static inline const char* Name() {
        return "const char[]";
    }

    static inline bool Check(State* s, int index) {
        return Support<const char*>::Check(s, index);
    }

    static inline char* Load(State* s, int index) = delete;

    static inline void Push(State* s, const char* p) {
        Support<const char*>::Push(s, p);
    }
};

template <class Trait, class Alloc>
struct Support<std::basic_string<char, Trait, Alloc>> : ValueCategory<std::basic_string<char, Trait, Alloc>, true, string_tag>{
    typedef std::basic_string<char, Trait, Alloc> value_type;

    static inline const char* Name() {
        return "std::string";
    }

    static inline bool Check(State* s, int index) {
        return Support<const char*>::Check(s, index);
    }

    static inline value_type Load(State* s, int index) {
        size_t len = 0;
        const char* str = lua_tolstring(s->GetLuaState(), index, &len);
        if (str && len)
            return value_type(str, len);
        return value_type();
    }

    static inline void Push(State* s, const value_type& str) {
        lua_pushlstring(s->GetLuaState(), str.c_str(), str.size());
    }
};

/* light user data support */
template <>
struct Support<void*> : ValueCategory<void*, true> {
    static inline const char* Name() {
        return "void*";
    }

    static inline bool Check(State* s, int index) {
        return lua_type(s->GetLuaState(), index) == LUA_TLIGHTUSERDATA;
    }

    static inline void* Load(State* s, int index) {
        return lua_touserdata(s->GetLuaState(), index);
    }

    static inline void Push(State* s, void* p) {
        if (p) lua_pushlightuserdata(s->GetLuaState(), p);
        else lua_pushnil(s->GetLuaState());
    }
};

template <>
struct Support<const void*> : ValueCategory<void*, true> {
    static inline const char* Name() {
        return "const void*";
    }

    static inline bool Check(State* s, int index) {
        return lua_type(s->GetLuaState(), index) == LUA_TLIGHTUSERDATA;
    }

    static inline const void* Load(State* s, int index) {
        return lua_touserdata(s->GetLuaState(), index);
    }

    static inline void Push(State* s, const void* p) = delete;
};

/* only used for check is nil */
template <>
struct Support<void> : ValueCategory<void, false> {
    static inline const char* Name() { return "void"; }
    static inline bool Check(State* s, int index) { return lua_type(s->GetLuaState(), index) == LUA_TNIL; }
    static inline void Load(State* s, int index) = delete;
    static inline void Push(State* s) = delete;
};

namespace internal {
    /* get parameter name and the lua type name */
    template <size_t Idx>
    struct ParamName {
        template <typename... Args>
        static inline void GetName(char* buff, size_t sz, State* s, int index) {
            constexpr size_t param_idx = sizeof...(Args) - Idx;
            typedef typename PurifyType<
                typename std::tuple_element<param_idx, std::tuple<Args...>>::type>::type type;
            int w = snprintf(buff, sz, "[%d] %s(%s), ", (int)(param_idx + 1),
                Support<type>::Name(), s->GetTypeName(index));
            if (w > 0 && w < sz)
                ParamName<Idx - 1>::template GetName(buff + w, sz - w, s, index + 1);
        }
    };

    template <>
    struct ParamName<1> {
        template <typename... Args>
        static inline void GetName(char* buff, size_t sz, State* s, int index) {
            constexpr size_t param_idx = sizeof...(Args) - 1;
            typedef typename PurifyType<
                typename std::tuple_element<param_idx, std::tuple<Args...>>::type>::type type;
            snprintf(buff, sz, "[%d] %s(%s)", (int)(param_idx + 1),
                Support<type>::Name(), s->GetTypeName(index));
        }
    };

    template <>
    struct ParamName<0> {
        template <typename... Args>
        static inline void GetName(char* buff, size_t sz, State* s, int index) {
            snprintf(buff, sz, "none");
        }
    };

    template <typename... Args>
    inline char* GetParameterNames(char* buff, size_t len, State* s, int index) {
        ParamName<sizeof...(Args)>::template GetName<Args...>(buff, len, s, 1);
        return buff;
    }

    template <typename Ty, typename std::enable_if<Support<Ty>::allow_nil, int>::type = 0>
    inline bool DoCheckParam(State* s, int index) {
        static_assert(!std::is_same<Support<Ty>::category, not_support_tag>::value, "not support");
        return lua_isnil(s->GetLuaState(), index) || Support<Ty>::Check(s, index);
    }

    template <typename Ty, typename std::enable_if<!Support<Ty>::allow_nil, int>::type = 0>
    inline bool DoCheckParam(State* s, int index) {
        static_assert(!std::is_same<Support<Ty>::category, not_support_tag>::value, "not support");
        return Support<Ty>::Check(s, index);
    }

    template <size_t Idx>
    struct ParamChecker {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            typedef typename PurifyType<
                typename std::tuple_element<sizeof...(Args) - Idx, std::tuple<Args...>>::type>::type type;
            return DoCheckParam<type>(s, index) &&
                ParamChecker<Idx-1>::template Do<Args...>(s, index + 1);
        }
    };

    template <>
    struct ParamChecker<1> {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            typedef typename PurifyType<
                typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type>::type type;
            return DoCheckParam<type>(s, index);
        }
    };

    template <>
    struct ParamChecker<0> {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            return true;
        }
    };

    template <typename... Args>
    inline bool CheckParameters(State* s, int index) {
        return ParamChecker<sizeof...(Args)>::template Do<Args...>(s, index);
    }

    template <typename Fy, typename Ry, typename... Args, size_t... Idxs>
    inline auto DoLuaCall(State* s, Fy f, index_sequence<Idxs...>) -> typename std::enable_if<!std::is_void<Ry>::value, int>::type {
        if (CheckParameters<Args...>(s, 1)) {
            s->Push(f(Support<typename PurifyType<Args>::type>::Load(s, Idxs + 1)...));
            return 1;
        } else {
            char buff[1024];
            luaL_error(s->GetLuaState(), "attemp to call export function failed, paramenter is not accpeted,\nparams{%s}",
                GetParameterNames<Args...>(buff, 1024, s, 1));
            return 0;
        }
    }

    template <typename Fy, typename Ry, typename... Args, size_t... Idxs>
    inline auto DoLuaCall(State* s, Fy f, index_sequence<Idxs...>) -> typename std::enable_if<std::is_void<Ry>::value, int>::type {
        if (CheckParameters<Args...>(s, 1)) {
            f(Support<typename PurifyType<Args>::type>::Load(s, Idxs + 1)...);
        } else {
            char buff[1024];
            luaL_error(s->GetLuaState(), "attemp to call export function failed, paramenter is not accpeted,\nparams{%s}",
                GetParameterNames<Args...>(buff, 1024, s, 1));
        }
        return 0;
    }

    template <typename Fy, typename Ry, typename... Args>
    inline int DoLuaCall(State* s, Fy f) {
        return DoLuaCall<Fy, Ry, Args...>(s, f, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ry, typename... Args>
    inline auto MakeFunc(Function f) -> typename std::enable_if<std::is_void<Ry>::value, std::function<Ry(Args...)>>::type{
        return [f](Args... args) mutable {
            f(std::tie(), args...);
        };
    }

    template <typename Ry, typename... Args>
    inline auto MakeFunc(Function f) -> typename std::enable_if<!std::is_void<Ry>::value, std::function<Ry(Args...)>>::type {
        return [f](Args... args) mutable -> Ry {
            Ry val;
            f(std::tie(val), args...);
            return std::move(val);
        };
    }
} // namespace internal

/* function support */
struct function_tag;

template <>
struct Support<int(lua_State*)> : ValueCategory<int(lua_State*), true, function_tag> {
    typedef int (*value_type)(lua_State*);

    static inline const char* Name() { return "cfunction"; }

    static inline bool Check(State* s, int index) {
        return lua_iscfunction(s->GetLuaState(), index);
    }

    static inline void Push(State* s, value_type f) {
        if (f) lua_pushcfunction(s->GetLuaState(), f);
        else lua_pushnil(s->GetLuaState());
    }

    static inline value_type Load(State* l, int index) {
        return lua_tocfunction(l->GetLuaState(), index);
    }
};

template <>
struct Support<int(State*)> : ValueCategory<int(State*), true, function_tag>{
    typedef int (*value_type)(State*);

    static inline const char* Name() { return "cfunction"; }

    static inline bool Check(State* s, int index) {
        return lua_tocfunction(s->GetLuaState(), index) == &Call;
    }

    static inline value_type Load(State* s, int index) {
        if (!Check(s, index))
            return nullptr;

        lua_getupvalue(s->GetLuaState(), index, 1);
        void* f = lua_touserdata(s->GetLuaState(), -1);
        lua_pop(s->GetLuaState(), 1);
        return static_cast<value_type>(f);
    }

    static inline void Push(State* s, value_type f) {
        if (!f) {
            lua_pushnil(s->GetLuaState());
        } else {
            lua_pushlightuserdata(s->GetLuaState(), static_cast<void*>(f));
            lua_pushcclosure(s->GetLuaState(), &Call, 1);
        }
    }

private:
    static int Call(lua_State* l) {
        auto* f = static_cast<value_type>(lua_touserdata(l, lua_upvalueindex(1)));
        return f(internal::GetState(l));
    };
};

template <typename Ry, typename... Args>
struct Support<Ry(Args...)> : ValueCategory<Ry(Args...), true, function_tag> {
    typedef Ry (*value_type)(Args...);
    typedef value_category_tag category;

    static inline bool Name() { return "xfunction"; }

    static inline bool Check(State* s, int index) {
        return lua_tocfunction(s->GetLuaState(), index) == &Call;
    }

    static inline value_type Load(State* s, int index) {
        if (!Check(s, index))
            return nullptr;

        lua_getupvalue(s->GetLuaState(), index, 1);
        void* f = lua_touserdata(s->GetLuaState(), -1);
        lua_pop(s->GetLuaState(), 1);
        return static_cast<value_type>(f);
    }

    static inline void Push(State* s, value_type f) {
        if (!f) {
            lua_pushnil(s->GetLuaState());
        } else {
            lua_pushlightuserdata(s->GetLuaState(), static_cast<void*>(f));
            lua_pushcclosure(s->GetLuaState(), &Call, 1);
        }
    }

private:
    int Call(lua_State* l) {
        auto f = static_cast<value_type>(lua_touserdata(l, lua_upvalueindex(1)));
        return internal::DoLuaCall<value_type, Ry, Args...>(internal::GetState(l), f);
    }
};

template <typename Ry, typename... Args>
struct Support<std::function<Ry(Args...)>> : ValueCategory<std::function<Ry(Args...)>, true, function_tag> {
    typedef std::function<Ry(Args...)> value_type;
    typedef internal::ObjData<value_type> ObjData;
    static inline const char* Name() { return "std::function"; }

    static inline bool Check(State* s, int index) {
        return lua_type(s->GetLuaState(), index) == LUA_TFUNCTION;
    }

    static inline value_type Load(State* s, int index) {
        int lty = lua_type(s->GetLuaState(), index);
        if (lty == LUA_TNIL || lty != LUA_TFUNCTION)
            return value_type();

        // same type
        if (lua_tocfunction(s->GetLuaState(), index) == &Call) {
            lua_getupvalue(s->GetLuaState(), index, 1);
            auto* d = static_cast<ObjData*>(lua_touserdata(s->GetLuaState(), -1));
            lua_pop(s->GetLuaState(), 1);
            return d->obj;
        }

        // other type funtion
        return internal::MakeFunc<Ry, Args...>(s->Load<Function>(index));
    }

    static inline void Push(State* s, const value_type& val) {
        if (!val) {
            lua_pushnil(s->GetLuaState());
        } else {
            s->state_.NewAloneObj<value_type>(val);
            lua_pushcclosure(s->GetLuaState(), &Call, 1);
        }
    }

private:
    static int Call(lua_State* l) {
        auto* d = static_cast<ObjData*>(lua_touserdata(l, lua_upvalueindex(1)));
        return internal::DoLuaCall<value_type&, Ry, Args...>(internal::GetState(l), d->obj);
    }
};

struct std_shared_ptr_tag {};

/* smart ptr support */
template <typename Ty>
struct Support<std::shared_ptr<Ty>> : ValueCategory<std::shared_ptr<Ty>, true, std_shared_ptr_tag> {
    static_assert(std::is_base_of<object_category_tag_, typename Support<Ty>::category>::value, "only support object");
    typedef ValueCategory<std::shared_ptr<Ty>, true, std_shared_ptr_tag> base_type_;
    typedef typename base_type_::value_type value_type;

    static const char* Name() { return "std::shared_ptr"; }

    static bool Check(State* s, int index) {
        auto* ud = s->state_.LoadRawUd(index);
        if (ud == nullptr || ud->minor != internal::UdMinor::SmartPtr)
            return false;

        auto* ptr = static_cast<internal::ObjUd*>(ud)->As<internal::SmartPtrData>();
        if (ptr->tag != base_type_::tag)
            return false;

        return s->state_.IsUd(ud, Support<Ty>::Desc());
    }

    static value_type Load(State* s, int index) {
        auto* ud = s->state_.LoadRawUd(index);
        if (ud == nullptr || ud->minor != internal::UdMinor::kSmartPtr)
            return value_type();

        auto* ptr = static_cast<internal::ObjUd*>(ud)->As<internal::SmartPtrData>();
        if (ptr->tag != base_type_::tag)
            return value_type();

        return value_type(*static_cast<value_type*>(ptr->data),
            (Ty*)_XLUA_TO_SUPER_PTR(ud->ptr, ud->desc, Support<Ty>::Desc()));
    }

    static void Push(State* s, const value_type& ptr) {
        if (!ptr)
            lua_pushnil(s->GetLuaState());
        else
            s->state_.PushSmartPtr(ptr.get(), ptr, base_type_::tag);
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
