#pragma once
#include "xlua_config.h"
#include "xlua_def.h"
#include <lua.hpp>
#include <type_traits>
#include <limits>
#include <string>

XLUA_NAMESPACE_BEGIN

template<size_t...>
struct index_sequence {};

template <size_t N, size_t... Indices>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, Indices...> {};

template<size_t... Indices>
struct make_index_sequence<0, Indices...> {
    typedef index_sequence<Indices...> type;
};

template <size_t N>
using make_index_sequence_t = typename make_index_sequence<N>::type;

namespace internal {
    template <typename Ty, bool>
    struct PurifyPtrType {
        typedef typename std::remove_cv<Ty>::type type;
    };

    template <typename Ty>
    struct PurifyPtrType<Ty, true> {
        typedef typename std::add_pointer<
            typename std::remove_cv<typename std::remove_pointer<Ty>::type>::type>::type type;
    };

    template <>
    struct PurifyPtrType<const char*, true> {
        typedef const char* type;
    };

    template <>
    struct PurifyPtrType<const volatile char*, true> {
        typedef const char* type;
    };

    template <>
    struct PurifyPtrType<const void*, true> {
        typedef const void* type;
    };

    template <>
    struct PurifyPtrType<const volatile void*, true> {
        typedef const void* type;
    };
} // namepace internal

/* purify type, supporter use raw type or pointer type */
template <typename Ty>
struct PurifyType {
    typedef typename internal::PurifyPtrType<
        typename std::remove_cv<Ty>::type, std::is_pointer<Ty>::value>::type type;
};

template <typename Ty>
struct PurifyType<Ty&> {
    typedef typename internal::PurifyPtrType<
        typename std::remove_cv<Ty>::type, std::is_pointer<Ty>::value>::type type;
};

template <typename Ty>
struct ObjectWrapper {
    ObjectWrapper(Ty* p) : ptr(p) {}
    ObjectWrapper(Ty& r) : ptr(&r) {}

    inline bool IsValid() const { return ptr != nullptr; }
    inline operator Ty&() const { return *ptr; }

private:
    Ty* ptr = nullptr;
};

template <>
struct ObjectWrapper<void> {
};

namespace internal {
    State* GetState(lua_State* l);
}
XLUA_NAMESPACE_END
