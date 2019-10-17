#pragma once
#include <lua.hpp>
#include <type_traits>
#include <limits>
#include <string>

#define XLUA_NAMESPACE_BEGIN namespace xlua {
#define XLUA_NAMESPACE_END }

XLUA_NAMESPACE_BEGIN

struct State {
    lua_State* GetState() { return nullptr; }
};

namespace internal {
    template <typename Ty>
    struct IsLarge {
        static constexpr bool is_integral = std::is_integral<Ty>::value;
        static constexpr bool value = std::numeric_limits<Ty>::digits > 32;
    };

    template <typename Ty> struct NumberSupport {
        typedef Ty type;

        static inline void Push(State* l, Ty value) {
            DoPush(l, value);
        }

        template <typename U>
        static inline bool Check(State* l, int index) {
            static_assert(!std::is_reference<U>::value, "xlua not support load reference number value");
            return lua_type(l->GetState(), index) == LUA_TNUMBER;
        }

        template <typename U>
        static inline Ty Load(State* l, int index) {
            return DoLoad<Ty>(l, index);
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
}

#define _XLUA_NUMBER_SUPPORT(Type)                                  \
template <> struct Support<Type> : internal::NumberSupport<Type> {  \
    static constexpr const char* Name() { return #Type; }           \
};

template <typename Ty> struct Support;

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

template <>
struct Support<char*> {
    typedef char* type;

    static inline const char* Name() {
        return "char*";
    }

    static inline void Push(State* l, const char* p) {
        if (p)
            lua_pushstring(l->GetState(), p);
        else
            lua_pushnil(l->GetState());
    }

    template <typename U>
    static inline bool Check(State* l, int index) {
        static_assert(!std::is_reference<U>::value, "xlua not support load reference string value");
        static_assert(!std::is_const<U>::value, "only support load const char*");
        return lua_isstring(l->GetState(), index) || lua_isnil(l->GetState(), index);
    }

    template <typename U>
    static inline const char* Load(State* l, int index) {
        return lua_tostring(l->GetState(), index);
    }
};

template <class Trait, class Alloc>
struct Support<std::basic_string<char, Trait, Alloc>> {
    typedef std::basic_string<char, Trait, Alloc>* type;

    static inline const char* Name() {
        return "std::string";
    }

    static inline void Push(State* l, const std::string& str) {
        lua_pushstring(l->GetState(), str.c_str());
    }

    template <typename U>
    static inline bool Check(State* l, int index) {
        static_assert(std::is_const<U>::value || !std::is_reference<U>::value, "xlua not support load reference string value");
        return Support<char*>::Check<const char*>(l, index);
    }

    template <typename U>
    static inline type Load(State* l, int index) {
        size_t len = 0;
        const char* s = lua_tolstring(l->GetState(), index, &len);
        if (s && len)
            return type(s, len);
        return type();
    }
};

template <>
struct Support<int(lua_State)> {
};


template <typename Ty>
struct Support {
    typedef Ty type;
    static_assert(true, "type is not declerared to lua");

    static inline const char* Name() {
        return "";
    }


};


XLUA_NAMESPACE_END
