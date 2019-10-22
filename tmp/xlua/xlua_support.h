#pragma once
#include "common.h"
#include "state.h"

XLUA_NAMESPACE_BEGIN

struct State {
    lua_State* GetState() { return nullptr; }
};

struct Variant {};
struct Table {};
struct Function {};
struct UserVar {};

template <typename Ty>
struct IsDeclear {
    static const bool = true;
};

namespace internal {
	template <typename Ty, bool>
    struct ExportSupport {
		typedef NoneCategory category;
    };

    template <typename Ty>
    struct ExportSupport<Ty, true> {
        typedef Ty value_type;
		typedef DeclaredCategory category;
		
		static inline bool Check(State* l, int index) {
			//TODO:
		}
		
		static inline const char* Name() {
			//TODO:
		}
    };
} // namespace internal

/* export type support */
template <typename Ty>
struct Support<Ty> : internal::ExportSupport<Ty, IsDeclear<Ty>::value> {
};

/* lua var support */
template <>
struct Support<Variant> {
	typedef Variant value_type;
	typedef ValueCategory category;
	
	static inline constexpr bool Check(State* l, int index) { return true; }
	static inline Variant Load(State* l, int index) { return l->LoadVar(index); }
	static inline void Push(State* l, const Variant& var) { l->PushVar(var); }
};

template <>
struct Support<Table> {
	typedef Table value_type;
	typedef ValueCategory category;
	
	static inline bool Check(State* l, int index) {
		int lty = lua_type(l->GetState(), index);
		return lty == LUA_TNIL || lty == LUA_TTABLE;
	}
	static inline Table Load(State* l, int index) { return l->LoadTable(index); }
	static inline void Push(State* l, const Table& var) { l->PushTable(var); }
};

template <>
struct Support<Function> {
	typedef Function value_type;
	typedef ValueCategory category;
	
	static inline bool Check(State* l, int index) {
		int lty = lua_type(l->GetState(), index);
		return lty == LUA_TNIL || lty == LUA_TFUNCTION;
	}
	static inline Function Load(State* l, int index) { return l->LoadFunc(index); }
	static inline void Push(State* l, const Function& var) { l->PushFunc(var); }
};

template <>
struct Support<UserVar> {
	typedef UserVar value_type;
	typedef ValueCategory category;
	
	static inline bool Check(State* l, int index) {
		int lty = lua_type(l->GetState(), index);
		return lty == LUA_TNIL || lty == LUA_TUSERDATA;
	}
	static inline UserVar Load(State* l, int index) { return l->LoadUserVar(index); }
	static inline void Push(State* l, const UserVar& var) { l->PushUserVar(var); }
};

/* nil support */
template <>
struct Support<std::nullptr_t> {
	typedef std::nullptr_t value_type;
	typedef ValueCategory category;
	
	static inline bool Check(State*, int) { return false; }
	static inline value_type Load(State*, int) = delete;
	static inline void Push(State* l) { lus_pushnil(l->GetState()); }
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
    static constexpr const char* Name() { return #Type; }           \
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

    static inline constexpr const char* Name() {
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

    static inline constexpr const char* Name() {
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
        return "string";
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
	
	static inline bool Check(State* l, int index) {
		int lty = lua_type(l->GetState(), index);
		return lty == LUA_TNIL || lty == LUA_TCFUNCTION;
	}
	
	static inline void Push(State* l, value_type f) {
		lua_pushcfunction(l, f);
	}
	
	static inline value_type Load(State* l, int index) {
		return lua_tocfunction(l->GetState(), index);
	}
	
private:
	static int Call(lua_State* l) {
		return 0;
	}
};

template <>
struct Support<int(State*)> {
	typedef int (*value_type)(State*);
	typedef ValueCategory category;
	
	static inline bool Check(State* l, int index) {
		return lua_tocfunction(l->GetState(), index) == &Call;
	}
	
	static inline value_type Load(State* l, int index) {
		//TODO:
	}
	
	static inline void Push(State* l, value_type f) {
		//TODO:
	}

private:
	static int Call(lua_State* l) {
		//TODO:
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

/* vector */
template <typename Ty>
struct Support<std::vector<Ty>> {
	typedef std::vector<Ty> value_type;
	typedef CollectionCategory category;
	
	static inline constexpr const char* Name() { return "vector"; }
	
	static inline ICollection* Proc() {
		static Proc sp;
		return &sp;
	}
	
private:
	struct Proc_ : ICollection {
		//TODO:
	};
};


XLUA_NAMESPACE_END
