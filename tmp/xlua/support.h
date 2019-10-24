#pragma once
#include "common.h"
#include "state.h"
#include <functional>
#include <vector>
#include <list>
#include <map>

XLUA_NAMESPACE_BEGIN

namespace internal {
    template <typename Ty, bool>
    struct ExportSupport {
        typedef NoneCategory category;
    };

    template <typename Ty>
    struct ExportSupport<Ty*, true> {
        typedef Ty value_type;
        typedef DeclaredCategory category;

        static inline const TypeDesc* Desc() { return xLuaGetTypeDesc(Identity<Ty>()); }
        static inline const char* Name() { return Desc()->name;  }
        static inline bool Check(State* l, int index) {
            return lua_isnil(l->GetState(), index) || l->state_.IsUd<Ty>(index);
        }
        static inline Ty* Load(State* l, int index) {
            return l->state_.LoadUd<Ty>(index);
        }
        static inline bool Push(State* l, Ty* ptr) {
            l->state_.PushUd(ptr);
        }
    };

    template <typename Ty>
    struct ExportSupport<Ty, true> {
        typedef Ty value_type;
        typedef DeclaredCategory category;
        typedef ExportSupport<Ty*, true> supporter;

        static inline const TypeInfo* Desc() { return supporter::Desc(); }
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
    };
} // namespace internal

/* export type support */
template <typename Ty>
struct Support : internal::ExportSupport<Ty, IsDeclaredType<Ty>::value> {
};

/* lua var support */
template <>
struct Support<Variant> {
    typedef Variant value_type;
    typedef ValueCategory category;
    static inline const char* Name() { return "xlua::Variant"; }
    static inline bool Check(State* l, int index) { return true; }
    //static inline Variant Load(State* l, int index) { return l->LoadVar(index); }
    //static inline void Push(State* l, const Variant& var) { l->PushVar(var); }
};

template <>
struct Support<Table> {
    typedef Table value_type;
    typedef ValueCategory category;

    static inline const char* Name() { return "xlua::Table"; }
    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetState(), index);
        return lty == LUA_TNIL || lty == LUA_TTABLE;
    }
    //static inline Table Load(State* l, int index) { return l->LoadTable(index); }
    //static inline void Push(State* l, const Table& var) { l->PushTable(var); }
};

template <>
struct Support<Function> {
    typedef Function value_type;
    typedef ValueCategory category;

    static inline const char* Name() { return "xlua::Function"; }
    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetState(), index);
        return lty == LUA_TNIL || lty == LUA_TFUNCTION;
    }
    //static inline Function Load(State* l, int index) { return l->LoadFunc(index); }
    //static inline void Push(State* l, const Function& var) { l->PushFunc(var); }
};

template <>
struct Support<UserVar> {
    typedef UserVar value_type;
    typedef ValueCategory category;

    static inline const char* Name() { return "xlua::UserVar"; }
    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetState(), index);
        return lty == LUA_TNIL || lty == LUA_TUSERDATA;
    }
    //static inline UserVar Load(State* l, int index) { return l->LoadUserVar(index); }
    //static inline void Push(State* l, const UserVar& var) { l->PushUserVar(var); }
};

/* nil support */
template <>
struct Support<std::nullptr_t> {
    typedef std::nullptr_t value_type;
    typedef ValueCategory category;

    static inline bool Check(State*, int) { return false; }
    static inline value_type Load(State*, int) = delete;
    static inline void Push(State* l, std::nullptr_t) { lua_pushnil(l->GetState()); }
};

/* number type support*/
namespace internal {
    template <typename Ty>
    struct IsLarge {
        static constexpr bool is_integral = std::is_integral<Ty>::value;
        static constexpr bool value = std::numeric_limits<Ty>::digits > 32;
    };

    template <typename Ty> struct NumberSupport {
        typedef Ty value_type;
        typedef ValueCategory category;

        static inline bool Check(State* l, int index) {
            return lua_type(l->GetState(), index) == LUA_TNUMBER;
        }

        static inline value_type Load(State* l, int index) {
            return DoLoad<value_type>(l, index);
        }

        static inline void Push(State* l, value_type value) {
            DoPush(l, value);
        }

    private:
        template <typename U, typename std::enable_if<std::is_floating_point<U>::value>::type>
        static inline void DoPush(State* l, U val) {
            lua_pushnumber(l->GetState(), static_cast<U>(val));
        }

        template <typename U, typename std::enable_if<std::is_integral<U>::value && !IsLarge<U>::value>::type>
        static inline void DoPush(State* l, U val) {
            lua_pushnumber(l->GetState(), val); }

        template <typename U, typename std::enable_if<std::is_integral<U>::value && IsLarge<U>::value>::type>
        static inline void DoPush(State* l, U val) {
            lua_pushinteger(l->GetState(), val);
        }

        template <typename U, typename std::enable_if<std::is_floating_point<U>::value>::type>
        static inline U DoLoad(State* l, int index) {
            return static_cast<U>(lua_tonumber(l->GetState(), index));
        }

        template <typename U, typename std::enable_if<std::is_integral<U>::value && !IsLarge<U>::value>::type>
        static inline U DoLoad(State* l, int index) {
            return static_cast<U>(lua_tonumber(l->GetState(), index));
        }

        template <typename U, typename std::enable_if<std::is_integral<U>::value && IsLarge<U>::value>::type>
        static inline U DoLoad(State* l, int index) {
            return static_cast<U>(lua_tointeger(l->GetState(), index));
        }
    };
} // namespace internal

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

/* string type support */
template <>
struct Support<const char*> {
    typedef const char* value_type;
    typedef ValueCategory category;

    static inline const char* Name() {
        return "const char*";
    }

    static inline bool Check(State* l, int index) {
        int lty = lua_type(l->GetState(), index);
        return lty == LUA_TNIL || lty == LUA_TSTRING;
    }

    static inline const char* Load(State* l, int index) {
        return lua_tostring(l->GetState(), index);
    }

    static inline void Push(State* l, const char* p) {
        if (p) lua_pushstring(l->GetState(), p);
        else lua_pushnil(l->GetState());
    }
};

template <>
struct Support<char*> {
    typedef char* value_type;
    typedef ValueCategory category;

    static inline const char* Name() {
        return "char*";
    }

    static inline bool Check(State* l, int index) {
        return Support<const char*>::Check(l, index);
    }

    static inline char* Load(State* l, int index) = delete;

    static inline void Push(State* l, char* p) {
        return Support<const char*>::Push(l, p);
    }
};

template <class Trait, class Alloc>
struct Support<std::basic_string<char, Trait, Alloc>> {
    typedef std::basic_string<char, Trait, Alloc>* value_type;
    typedef ValueCategory category;

    static inline const char* Name() {
        return "std::string";
    }

    static inline bool Check(State* l, int index) {
        return Support<const char*>::Check(l, index);
    }

    static inline value_type Load(State* l, int index) {
        size_t len = 0;
        const char* s = lua_tolstring(l->GetState(), index, &len);
        if (s && len)
            return value_type(s, len);
        return value_type();
    }

    static inline void Push(State* l, const value_type& str) {
        lua_pushlstring(l->GetState(), str.c_str(), str.size());
    }
};

/* light user data support */
template <>
struct Support<void*> {
    typedef char* value_type;
    typedef ValueCategory category;

    static inline const char* Name() {
        return "void*";
    }

    static inline bool Check(State* l, int index) {
        auto lty = lua_type(l->GetState(), index);
        return lty == LUA_TNIL || lty == LUA_TLIGHTUSERDATA;
    }

    static inline void* Load(State* l, int index) {
        return lua_touserdata(l->GetState(), index);
    }

    static inline void Push(State* l, void* p) {
        if (p) lua_pushlightuserdata(l->GetState(), p);
        else lua_pushnil(l->GetState());
    }
};

/* function support */
template <typename Ry, typename... Args>
struct Support<std::function<Ry(Args...)>> {
    typedef std::function<Ry(Args...)> value_type;
    typedef ValueCategory category;

    static inline const char* Name() { return "function"; }

    static inline bool Check(State* l, int index) {
        if (lua_iscfunction(l->GetState(), index))
            return lua_tocfunction(l->GetState(), index) == &Call;

        int lty = lua_type(l->GetState(), index);
        return lty == LUA_TNIL || lty == LUA_TFUNCTION
    }

    static value_type Load(State* l, int index) {
        //TODO:
    }

    static inline void Push(State* l, const value_type& val) {
        if (!val) {
            lua_pushnil(l->GetState());
        } else {
            //TODO:
        }
    }
};

template <>
struct Support<int(lua_State*)> {
    typedef int (*value_type)(lua_State*);
    typedef ValueCategory category;

    static inline const char* Name() { return "cfunction"; }

    static inline bool Check(State* l, int index) {
        return lua_isnil(l->GetState(), index) ||
            lua_iscfunction(l->GetState(), index);
    }

    static inline void Push(State* l, value_type f) {
        lua_pushcfunction(l->GetState(), f);
    }

    static inline value_type Load(State* l, int index) {
        return lua_tocfunction(l->GetState(), index);
    }
};

template <>
struct Support<int(State*)> {
    typedef int (*value_type)(State*);
    typedef ValueCategory category;

    static inline const char* Name() { return "cfunction"; }

    static inline bool Check(State* l, int index) {
        return lua_tocfunction(l->GetState(), index) == &Call;
    }

    static inline value_type Load(State* l, int index) {
        //TODO:
        return nullptr;
    }

    static inline void Push(State* l, value_type f) {
        //TODO:
    }

private:
    static int Call(lua_State* l) {
        //TODO:
        return 0;
    };
};

template <typename Ry, typename... Args>
struct Support<Ry(Args...)> {
    typedef Ry (*value_type)(Args...);
    typedef ValueCategory category;

    static inline bool Check(State* l, int index) { return false; }
    static value_type Load(State* l, int index) = delete;

    static inline void Push(State* l, value_type f) {
        //TODO:
    }
};

struct std_shared_ptr_tag {};

/* smart ptr support */
template <typename Ty>
struct Support<std::shared_ptr<Ty>> {
    static_assert(std::is_base_of<_ObjectCategory, Support<Ty>::category>::value, "only support object");

    typedef std::shared_ptr<Ty> value_type;
    typedef ValueCategory category;

    static const size_t tag = typeid(std_shared_ptr_tag).hash_code();

    static const char* Name() { return "std::shared_ptr"; }

    static bool Check(State* l, int index) {
        if (lua_isnil(l->GetState(), index))
            return true;

        auto* ud = l->LoadUserData(index);
        if (ud == nullptr || ud->category != UserDataCategory::SmartPtr)
            return false;

        if (static_cast<inernal::SmartPtrData*>(ud)->tag != tag)
            return false;

        return l->IsUd(ud, Support<Ty>::Desc());
    }

    static value_type Load(State* l, int index) {
        if (lua_isnil(l->GetState(), index))
            return value_type();

        auto* ud = l->LoadUserData(index);
        if (ud == nullptr || ud->category != UserDataCategory::SmartPtr)
            return value_type();

        auto* sud = static_cast<inernal::SmartPtrData*>(ud);
        if (sud->tag != tag)
            return value_type();
        //TODO:
        return value_type(*static_cast<value_type*>(sud->data),
            (Ty*)_XLUA_TO_SUPER_PTR(info, ud->obj_, ud->info_));
    }

    static void Push(State* l, const value_type& ptr) {
        if (!ptr)
            lua_pushnil(l->GetState());
        else
            l->state_.PushUd(ptr, ptr->get(), tag);
    }
};

namespace internal {
    template <typename Ty>
    struct UniqueObj {
        inline Ty& Instance() { return obj; }
    private:
        static Ty obj;
    };
    template <typename Ty> static Ty UniqueObj<Ty>::obj;

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

        int Length(void* obj, State* l) override {
            return 0;
        }
    };

    template <typename Cy, typename Pc>
    struct CollectionSupport {
        typedef Cy value_type;
        typedef CollectionCategory category;

        static inline ICollection* Desc() { return &UniqueObj<Pc>::Instance(); }

        static inline bool Check(State* l, int index) {
            return Support<Cy*>::Load(l, index) != nullptr;
        }

        static inline ObjectWrapper<Cy> Load(State* l, index) {
            return ObjectWrapper<Cy>(Support<Cy*>::Load(l, index));
        }

        static inline void Push(State* l, const value_type& obj) {
            l->state_.PushUd(obj);
        }
    };

    template <typename Cy, typename Pc>
    struct CollectionSupport<Cy*, Pc> {
        typedef Cy* value_type;
        typedef CollectionCategory category;

        static inline ICollection* Desc() { return &UniqueObj<Pc>::Instance(); }

        static inline bool Check(State* l, int index) {
            return lua_isnil(l->GetState(), index) || l->state_.IsUd<Cy>(index);
        }

        static inline value_type Load(State* l, index) {
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
