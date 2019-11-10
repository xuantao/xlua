#pragma once
XLUA_NAMESPACE_BEGIN

template <typename Cat, typename Ty, bool AllowNil>
struct SupportCategory {
    typedef Cat category;   // value, object or not support
    typedef Ty value_type;  // load value type
    static constexpr bool allow_nil = AllowNil; // load as function parameter is allow nil
};

template <typename Ty, bool AllowNil>
struct ValueCategory : SupportCategory<value_category_tag, Ty, AllowNil> {};

template <typename Ty>
struct ObjectCategory : SupportCategory<object_category_tag_, Ty, std::is_pointer<Ty>::value> {};

namespace internal {
    template <typename Ty>
    struct IsLarge {
        static constexpr bool is_integer = std::numeric_limits<Ty>::is_integer;
        static constexpr bool value = std::numeric_limits<Ty>::digits > 32;
    };

    struct void_tag {};
    struct enum_tag {};
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

    /* object value */
    template <typename Ty>
    struct ObjectSupport : ObjectCategory<ObjectWrapper<Ty>> {
        typedef Support<Ty*> supporter;

        static inline bool Check(State* l, int index) {
            return supporter::Load(l, index) != nullptr;
        }
        static inline ObjectWrapper<Ty> Load(State* l, int index) {
            return ObjectWrapper<Ty>(supporter::Load(l, index));
        }
        static inline void Push(State* l, const Ty& obj) {
            l->state_.PushUd(obj, supporter::TypeInfo());
        }
        static inline void Push(State* l, Ty&& obj) {
            l->state_.PushUd(std::move(obj), supporter::TypeInfo());
        }
    };

    /* object pointer */
    template <typename Ty>
    struct ObjectSupport<Ty*> : ObjectCategory<Ty*> {
        typedef Support<Ty*> supporter;

        static inline bool Check(State* l, int index) {
            return l->state_.IsUd<Ty>(index, supporter::TypeInfo());
        }
        static inline Ty* Load(State* l, int index) {
            return l->state_.LoadUd<Ty>(index, supporter::TypeInfo());
        }
        static inline void Push(State* l, Ty* ptr) {
            l->state_.PushUd(ptr, supporter::TypeInfo());
        }
    };

    template <typename Ty, typename Tag>
    struct ExportSupport : SupportCategory<not_support_tag, void, true> {
    };

    /* declared type support */
    template <typename Ty>
    struct ExportSupport<Ty, declared_tag> : ObjectSupport<Ty> {
        static inline const TypeDesc* TypeInfo() {
            return xLuaGetTypeDesc(Identity<typename std::remove_pointer<Ty>::type>());
        }
        static inline const char* Name() { return TypeInfo()->name;  }
    };

    /* enum support */
    template <typename Ty>
    struct ExportSupport<Ty, enum_tag> : ValueCategory<Ty, false> {
        typedef Support<typename std::underlying_type<Ty>::type> supporter;
        typedef typename supporter::value_type underlying_type;

        static inline const char* Name() { return typeid(Ty).name(); }
        static inline bool Check(State* l, int index) {
            return supporter::Check(l, index);
        }
        static inline Ty Load(State* l, int index) {
            return static_cast<Ty>(supporter::Load(l, index));
        }
        static inline void Push(State* l, Ty value) {
            supporter::Push(l, static_cast<underlying_type>(value));
        }
    };

    /* number value support */
    template <typename Ty>
    struct NumberSupport : ValueCategory<Ty, false> {
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

    /* collection support */
    template <typename Ty, typename CollTy>
    struct CollectionSupport : ObjectSupport<Ty> {
        static inline ICollection* TypeInfo() { return &coll_; }
        static inline const char* Name() { return TypeInfo()->Name(); }
    private:
        static CollTy coll_;
    };
    template <typename Ty, typename CollTy> CollTy CollectionSupport<Ty, CollTy>::coll_;

    template <typename Ty, typename CollTy>
    struct CollectionSupport<Ty*, CollTy> : ObjectSupport<Ty*> {
        typedef CollectionSupport<Ty, CollTy> supporter;

        static inline ICollection* TypeInfo() { return supporter::TypeInfo(); }
        static inline const char* Name() { return supporter::Name(); }
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
_XLUA_NUMBER_SUPPORT(int)
_XLUA_NUMBER_SUPPORT(unsigned int)
_XLUA_NUMBER_SUPPORT(long)
_XLUA_NUMBER_SUPPORT(unsigned long)
_XLUA_NUMBER_SUPPORT(long long)
_XLUA_NUMBER_SUPPORT(unsigned long long)
_XLUA_NUMBER_SUPPORT(float)
_XLUA_NUMBER_SUPPORT(double)

/* boolean type support */
template <>
struct Support<bool> : ValueCategory<bool, true> {
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
template <>
struct Support<char*> : ValueCategory<const char*, true> {
    static inline const char* Name() {
        return "char*";
    }
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

// char array is act as const char*
template <size_t N>
struct Support<char[N]> : ValueCategory<char*, true> {
    static_assert(N > 0, "char array size must greater than 0");

    static inline const char* Name() {
        return "char[]";
    }
    static inline bool Check(State* s, int index) {
        return Support<char*>::Check(s, index);
    }
    static inline void Push(State* s, const char* p) {
        Support<char*>::Push(s, p);
    }

    static inline char* Load(State* s, int index) = delete;
};

//template <size_t N>
//struct Support<const char[N]> : ValueCategory<const char*, true> {
//    static_assert(N > 0, "char array size must greater than 0");
//    static inline const char* Name() {
//        return "const char[]";
//    }
//
//    static inline bool Check(State* s, int index) {
//        return Support<const char*>::Check(s, index);
//    }
//
//    static inline char* Load(State* s, int index) = delete;
//
//    static inline void Push(State* s, const char* p) {
//        Support<const char*>::Push(s, p);
//    }
//};

template <class Trait, class Alloc>
struct Support<std::basic_string<char, Trait, Alloc>> : ValueCategory<std::basic_string<char, Trait, Alloc>, true>{
    typedef std::basic_string<char, Trait, Alloc> value_type;

    static inline const char* Name() { return "std::string"; }
    static inline bool Check(State* s, int index) {
        return Support<char*>::Check(s, index);
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

/* only used for check is nil */
template <>
struct Support<void> : ValueCategory<void, false> {
    static inline const char* Name() { return "void"; }
    static inline bool Check(State* s, int index) { return lua_type(s->GetLuaState(), index) == LUA_TNIL; }
    static inline void Load(State* s, int index) = delete;
    static inline void Push(State* s) = delete;
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

    static inline void Push(State* s, const void* p) {
        if (p) lua_pushlightuserdata(s->GetLuaState(), const_cast<void*>(p));
        else lua_pushnil(s->GetLuaState());
    }
};

namespace internal {
    /* get parameter name and the lua type name */
    template <size_t Idx>
    struct ParamName {
        template <typename... Args>
        static inline void GetName(char* buff, size_t sz, State* s, int index) {
            constexpr size_t param_idx = sizeof...(Args) - Idx;
            using supporter = typename SupportTraits<
                typename std::tuple_element<param_idx, std::tuple<Args...>>::type>::supporter;
            int w = snprintf(buff, sz, "[%d] %s(%s), ", (int)(param_idx + 1),
                supporter::Name(), s->GetTypeName(index));
            if (w > 0 && w < sz)
                ParamName<Idx - 1>::template GetName(buff + w, sz - w, s, index + 1);
        }
    };

    template <>
    struct ParamName<1> {
        template <typename... Args>
        static inline void GetName(char* buff, size_t sz, State* s, int index) {
            constexpr size_t param_idx = sizeof...(Args) - 1;
            using supporter = typename SupportTraits<
                typename std::tuple_element<param_idx, std::tuple<Args...>>::type>::supporter;
            snprintf(buff, sz, "[%d] %s(%s)", (int)(param_idx + 1),
                supporter::Name(), s->GetTypeName(index));
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

    template <typename Ty, typename std::enable_if<SupportTraits<Ty>::is_allow_nil, int>::type = 0>
    inline bool DoCheckParam(State* s, int index) {
        static_assert(SupportTraits<Ty>::is_support, "not xlua support type");
        using supporter = typename SupportTraits<Ty>::supporter;
        return lua_isnil(s->GetLuaState(), index) || supporter::Check(s, index);
    }

    template <typename Ty, typename std::enable_if<!SupportTraits<Ty>::is_allow_nil, int>::type = 0>
    inline bool DoCheckParam(State* s, int index) {
        static_assert(SupportTraits<Ty>::is_support, "not xlua support type");
        using supporter = typename SupportTraits<Ty>::supporter;
        return supporter::Check(s, index);
    }

    template <size_t Idx>
    struct ParamChecker {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            using type = typename std::tuple_element<sizeof...(Args) - Idx, std::tuple<Args...>>::type;
            return DoCheckParam<type>(s, index) &&
                ParamChecker<Idx-1>::template Do<Args...>(s, index + 1);
        }
    };

    template <>
    struct ParamChecker<1> {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            using type = typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type;
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
            s->Push(f(SupportTraits<Args>::supporter::Load(s, Idxs + 1)...));
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
            f(SupportTraits<Args>::supporter::Load(s, Idxs + 1)...);
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
template <>
struct Support<int(lua_State*)> : ValueCategory<int(lua_State*), true> {
    typedef int (*value_type)(lua_State*);

    static inline const char* Name() { return "lua_cfunction"; }
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
struct Support<int(State*)> : ValueCategory<int(State*), true>{
    typedef int (*value_type)(State*);

    static inline const char* Name() { return "xlua_cfunction"; }

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
struct Support<Ry(Args...)> : ValueCategory<Ry(Args...), true> {
    typedef Ry (*value_type)(Args...);
    typedef value_category_tag category;

    static inline bool Name() { return "cfunction"; }

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
struct Support<std::function<Ry(Args...)>> : ValueCategory<std::function<Ry(Args...)>, true> {
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
struct Support<std::shared_ptr<Ty>> : ValueCategory<std::shared_ptr<Ty>, true> {
    static_assert(SupportTraits<Ty>::is_obj_type, "shared_ptr only support object type");
    typedef std::shared_ptr<Ty> value_type;
    typedef typename SupportTraits<Ty>::supporter supporter;

    static const char* Name() { return "std::shared_ptr"; }

    static bool Check(State* s, int index) {
        auto* ud = s->state_.LoadRawUd(index);
        if (ud == nullptr || ud->minor != internal::UdMinor::SmartPtr)
            return false;

        auto* ptr = static_cast<internal::ObjUd*>(ud)->As<internal::SmartPtrData>();
        if (ptr->tag != tag_)
            return false;

        return s->state_.IsUd<Ty>(ud, supporter::TypeInfo());
    }

    static value_type Load(State* s, int index) {
        auto* ud = s->state_.LoadRawUd(index);
        if (ud == nullptr || ud->minor != internal::UdMinor::kSmartPtr)
            return value_type();

        auto* ptr = static_cast<internal::ObjUd*>(ud)->As<internal::SmartPtrData>();
        if (ptr->tag != tag_)
            return value_type();

        auto* obj = internal::As(ud, supporter::TypeInfo());
        return value_type(*static_cast<value_type*>(ptr->data), (Ty*)obj);
    }

    static void Push(State* s, const value_type& ptr) {
        if (!ptr)
            lua_pushnil(s->GetLuaState());
        else
            s->state_.PushSmartPtr(ptr.get(), ptr, tag_, supporter::TypeInfo());
    }

    static void Push(State* l, value_type&& ptr) {
        if (!ptr)
            lua_pushnil(l->GetLuaState());
        else
            l->state_.PushSmartPtr(ptr.get(), std::move(ptr), tag_, supporter::TypeInfo());
    }

private:
    static size_t tag_;
};
template <typename Ty>
size_t Support<std::shared_ptr<Ty>>::tag_ = typeid(std_shared_ptr_tag).hash_code();

/* std array support */
template <typename Ty, size_t N>
struct Support<std::array<Ty, N>> : ValueCategory<std::array<Ty, N>, true> {
    //TODO:
};

namespace internal {
    /* vector collection processor */
    template <typename VecTy>
    struct VectorCollection : ICollection {
        typedef VecTy vector_type;
        typedef typename VecTy::value_type value_type;
        typedef typename SupportTraits<value_type>::supporter supporter;

        const char* Name() override { return "std::vector"; }

        int Index(void* obj, State* s) override {
            auto* vec = As(obj);
            int idx = LoadIndex(s);
            if (idx == 0 || !CheckRange(s, idx, vec->size()))
                return 0;

            s->Push(vec->at(idx - 1));
            return 1;
        }

        int NewIndex(void* obj, State* s) override {
            auto* vec = As(obj);
            int idx = LoadIndex(s);
            if (idx == 0 || !CheckRange(s, idx, vec->size() + 1) || !CheckValue(s))
                return 0;

            if (idx == (int)vec->size() + 1)
                vec->push_back(supporter::Load(s, 3));
            else
                (*vec)[idx - 1] = supporter::Load(s, 3);
            return 0;
        }

        int Insert(void* obj, State* s) override {
            auto* vec = As(obj);
            int idx = LoadIndex(s);
            if (idx == 0 || !CheckRange(s, idx, vec->size()) || !CheckValue(s))
                return 0;

            vec->insert(vec->begin() + (idx - 1), supporter::Load(s, 3));
            s->Push(true);
            return 1;
        }

        int Remove(void* obj, State* s) override {
            auto* vec = As(obj);
            int idx = LoadIndex(s);
            if (idx == 0 || !CheckRange(s, idx, vec->size()))
                return 0;

            vec->erase(vec->begin() + (idx - 1));
            return 0;
        }

        int Iter(void* obj, State* s) override {
            lua_pushcfunction(s->GetLuaState(), &sIter);
            lua_pushlightuserdata(s->GetLuaState(), obj);
            lua_pushnumber(s->GetLuaState(), 0);
            return 3;
        }

        int Length(void* obj) override {
            return (int)As(obj)->size();
        }

        void Clear(void* obj) override {
            As(obj)->clear();
        }

    protected:
        static inline vector_type* As(void* obj) { return static_cast<vector_type*>(obj); }

        inline int LoadIndex(State* s) {
            if (!lua_isnumber(s->GetLuaState(), 2)) {
                luaL_error(s->GetLuaState(), "vector only accept number index key");
                return 0;
            }

            int idx = s->Load<int>(2);
            if (idx <= 0) {
                luaL_error(s->GetLuaState(), "vector index must greater than '0'");
                return 0;
            }
            return idx;
        }

        inline bool CheckRange(State* s, int idx, size_t sz) {
            if (idx > 0 && idx <= (int)sz)
                return true;

            luaL_error(s->GetLuaState(), "vector index is out of range");
            return false;
        }

        inline bool CheckValue(State* s) {
            if (internal::DoCheckParam<value_type>(s, 3))
                return true;

            luaL_error(s->GetLuaState(), "vector value is not allow");
            return false;
        }

        static int sIter(lua_State* l) {
            auto* obj = As(lua_touserdata(l, 1));
            int idx = (int)lua_tonumber(l, 2);
            if (idx < obj->size()) {
                lua_pushnumber(l, idx + 1);                 // next key
                internal::GetState(l)->Push(obj->at(idx));  // value
            } else {
                lua_pushnil(l);
                lua_pushnil(l);
            }
            return 2;
        }
    };

    /* map collection processor */
    template <typename MapTy>
    struct MapCollection : ICollection {
        typedef MapTy map_type;
        typedef typename map_type::iterator iterator;
        typedef typename map_type::key_type key_type;
        typedef typename map_type::mapped_type value_type;
        typedef typename SupportTraits<key_type>::supporter key_supporter;
        typedef typename SupportTraits<value_type>::supporter value_supporter;

        const char* Name() override { return "std::map"; }

        int Index(void* obj, State* s) override {
            if (!CheckKey(s))
                return 0;

            key_type key = key_supporter::Load(s, 2);
            auto* map = As(obj);
            auto it = map->find(key);

            if (it == map->end())
                s->PushNil();
            else
                s->Push(it->second);
            return 1;
        }

        int NewIndex(void* obj, State* s) override {
            if (!CheckKey(s))
                return 0;

            key_type key = key_supporter::Load(s, 2);
            auto* map = As(obj);
            if (s->IsNil(3)) {
                map->erase(key);
            } else if (CheckValue(s)) {
                auto it = map->find(key);
                if (it == map->end())
                    map->insert(std::make_pair(key, (value_type)value_supporter::Load(s, 3)));
                else
                    it->second = value_supporter::Load(s, 3);
            }
            return 0;
        }

        int Insert(void* obj, State* s) override {
            if (!CheckKey(s) || !CheckValue(s))
                return 0;

            key_type key = key_supporter::Load(s, 2);
            auto* map = As(obj);
            if (map->cend() == map->find(key)) {
                map->insert(std::make_pair(key, (value_type)value_supporter::Load(s, 3)));
                s->Push(true);
            } else {
                s->Push(false);
            }
            return 1;
        }

        int Remove(void* obj, State* s) override {
            if (CheckKey(s)) {
                key_type key = key_supporter::Load(s, 2);
                auto* map = As(obj);
                map->erase(key);
            }
            return 0;
        }

        int Iter(void* obj, State* s) override {
            lua_pushcfunction(s->GetLuaState(), &sIter);
            lua_pushlightuserdata(s->GetLuaState(), obj);
            s->state_.NewAloneObj<iterator>(As(obj)->begin());
            return 0;
        }

        int Length(void* obj) override {
            return (int)As(obj)->size();
        }

        void Clear(void* obj) override {
            As(obj)->clear();
        }

    protected:
        static inline map_type* As(void* obj) { return static_cast<map_type*>(obj); }

        inline bool CheckKey(State* s) {
            if (key_supporter::Check(s, 2))
                return true;
            luaL_error(s->GetLuaState(), "vector index is out of range");
            return false;
        }

        inline bool CheckValue(State* s) {
            if (internal::DoCheckParam<value_type>(s, 3))
                return true;
            luaL_error(s->GetLuaState(), "vector value is not allow");
            return false;
        }

        static int sIter(lua_State* l) {
            auto* obj = As(lua_touserdata(l, 1));
            auto* data = static_cast<internal::ObjData<iterator>*>(lua_touserdata(l, 2));
            if (data->obj != obj->end()) {
                lua_pushvalue(l, 1);
                value_supporter::Push(internal::GetState(l), data->obj->second);
                ++ data->obj;   // move iterator
            } else {
                lua_pushnil(l);
                lua_pushnil(l);
            }
            return 2;
        }
    };
} // namespace internal

template <typename Ty, typename Alloc>
struct Support<std::vector<Ty, Alloc>>
    : internal::CollectionSupport<std::vector<Ty, Alloc>, internal::VectorCollection<std::vector<Ty, Alloc>>> {
};

template <typename Ty, typename Alloc>
struct Support<std::vector<Ty, Alloc>*>
    : internal::CollectionSupport<std::vector<Ty, Alloc>*, internal::VectorCollection<std::vector<Ty, Alloc>>> {
};

template <typename KeyType, typename ValueType, typename PrTy, typename Alloc>
struct Support<std::map<KeyType, ValueType, PrTy, Alloc>>
    : internal::CollectionSupport<std::map<KeyType, ValueType, PrTy, Alloc>,
        internal::MapCollection<std::map<KeyType, ValueType, PrTy, Alloc>>> {
};

template <typename KeyType, typename ValueType, typename PrTy, typename Alloc>
struct Support<std::map<KeyType, ValueType, PrTy, Alloc>*>
    : internal::CollectionSupport<std::map<KeyType, ValueType, PrTy, Alloc>*,
        internal::MapCollection<std::map<KeyType, ValueType, PrTy, Alloc>>> {
};

XLUA_NAMESPACE_END
