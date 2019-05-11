#pragma once
#include "../xlua_def.h"
#include <type_traits>

XLUA_NAMESPACE_BEGIN

namespace detail {
    template<size_t...>
    struct index_sequence {};

    template <size_t N, size_t... Indices>
    struct make_index_sequence : make_index_sequence<N - 1, N - 1, Indices...> {};

    template<size_t... Indices>
    struct make_index_sequence<0, Indices...>
    {
        typedef index_sequence<Indices...> type;
    };

    template <size_t N>
    using make_index_sequence_t = typename make_index_sequence<N>::type;

    template<class Ty>
    struct is_null_pointer : std::is_same<std::nullptr_t, typename std::remove_cv<Ty>::type> {};

    template <typename Ty>
    struct AddReference {
        typedef Ty& type;
    };

    template <typename Ty>
    struct AddReference<Ty&> {
        typedef Ty& type;
    };

    template <>
    struct AddReference<void> {
        typedef void type;
    };

    template<typename Ty>
    struct HasObjIndex {
        template <typename U> static auto Check(int)->decltype(std::declval<U>().xlua_obj_index_);
        template <typename U> static auto Check(...)->std::false_type;

        static constexpr bool value = std::is_same<decltype(Check<Ty>(0)), xLuaIndex>::value;
    };

    template <typename Ty>
    struct IsLuaType {
        template <typename U> static auto Check(int)->decltype(xLuaGetTypeInfo(Identity<U>()), std::true_type());
        template <typename U> static auto Check(...)->std::false_type;

        static constexpr bool value = decltype(Check<Ty>(0))::value;
    };

    template <typename Ty>
    struct IsExtendLoad {
        template <typename U> static auto Check(int)->decltype(xLuaLoad(std::declval<xLuaState*>(), (int)0, Identity<U>()), std::true_type());
        template <typename U> static auto Check(...)->std::false_type;

        static constexpr bool value = decltype(Check<Ty>(0))::value;
    };

    template <typename Ty>
    struct IsExtendPush {
        template <typename U> static auto Check(int)->decltype(xLuaPush(std::declval<xLuaState*>(), std::declval<const U&>()), std::true_type());
        template <typename U> static auto Check(...)->std::false_type;

        static constexpr bool value = decltype(Check<Ty>(0))::value;
    };

    template <typename Ty>
    struct IsExtendType {
        template <typename U> static auto Check(int)->decltype(xLuaIsType(std::declval<xLuaState*>(), (int)0, Identity<U>()), std::true_type());
        template <typename U> static auto Check(...)->std::false_type;

        static constexpr bool value = decltype(Check<Ty>(0))::value;
    };

    template <typename Ty>
    struct IsWeakObjPtr {
        static constexpr bool value = std::is_base_of<XLUA_WEAK_OBJ_BASE_TYPE, Ty>::value;
    };

    struct tag_unknown {};
    struct tag_weakobj {};
    struct tag_obj_index {};
    struct tag_declared {};
    struct tag_extend {};
    struct tag_enum {};

    template <typename Ty>
    inline auto GetTypeInfoImpl() -> typename std::enable_if<IsLuaType<Ty>::value, const TypeInfo*>::type {
        return xLuaGetTypeInfo(Identity<Ty>());
    }

    template <typename Ty>
    inline auto GetTypeInfoImpl() -> typename std::enable_if<std::is_void<Ty>::value, const TypeInfo*>::type {
        return nullptr;
    }
} // namespace detail

XLUA_NAMESPACE_END
