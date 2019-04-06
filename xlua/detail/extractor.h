#pragma once
#include "common.h"

XLUA_NAMESPACE_BEGIN

namespace detail {
    /* �������
     * �����������Զ�ת�����
     * NormalTag  ->  AnyTag
     * ConstTag   ->  AnyTag
     * NormalTag <--> ConstTag
    */
    struct AnyTag;
    struct NormalTag;
    struct ConstTag;

    struct AnyTag
    {
        friend struct NormalTag;
        friend struct ConstTag;

    private:
        AnyTag() = default;
    };

    struct NormalTag
    {
        NormalTag() = default;
        NormalTag(const ConstTag&) {}

        operator AnyTag() const { return AnyTag(); }
    };

    struct ConstTag
    {
        ConstTag() = default;
        ConstTag(const NormalTag&) {}

        operator AnyTag() const { return AnyTag(); }
    };

    template <typename Rty, typename Cty, typename... Args>
    struct MemberFuncType
    {
        typedef Rty(Cty::*type)(Args...);
        typedef Rty(Cty::*const_type)(Args...) const;
    };

    template <typename Rty, typename Cty>
    struct MemberDataType
    {
        typedef Rty Cty::* type;
    };

    template <typename Rty, typename... Args>
    struct FuncType
    {
        typedef Rty(*type)(Args...);
    };

    /* ��ȡĿ�꺯��ָ��
     * ��Ҫԭ����ͨ�����ñ�����ƥ�亯�����õ����ȼ�����ȡ
    */
    template <typename... Args>
    struct Extractor
    {
        template <typename Cty, typename Rty>
        static inline auto extract(ConstTag, Rty(Cty::*const_func)(Args...) const) -> typename MemberFuncType<Rty, Cty, Args...>::const_type
        {
            return const_func;
        }

        template <typename Cty, typename Rty>
        static inline auto extract(NormalTag, Rty(Cty::*func)(Args...)) -> typename MemberFuncType<Rty, Cty, Args...>::type
        {
            return func;
        }

        template <typename Rty>
        static inline auto extract(AnyTag, Rty(*global_func)(Args...)) -> typename FuncType<Rty, Args...>::type
        {
            return global_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, xLuaState*>::const_type extract(ConstTag, Rty(Cty::*const_func)(xLuaState*) const)
        {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return const_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, xLuaState*>::type extract(NormalTag, Rty(Cty::*func)(xLuaState*))
        {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return func;
        }
    };

    /* ��ȡ���������汾�ĺ���
    */
    template <>
    struct Extractor<void>
    {
        template <typename Cty, typename Rty>
        static inline auto extract(ConstTag, Rty(Cty::*const_func)(void) const) -> typename MemberFuncType<Rty, Cty>::const_type
        {
            return const_func;
        }

        template <typename Cty, typename Rty>
        static inline auto extract(NormalTag, Rty(Cty::*func)(void)) -> typename MemberFuncType<Rty, Cty>::type
        {
            return func;
        }

        template <typename Rty>
        static inline auto extract(AnyTag, Rty(*global_func)(void)) -> typename FuncType<Rty>::type
        {
            return global_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, xLuaState*>::const_type extract(ConstTag, Rty(Cty::*const_func)(xLuaState*) const)
        {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return const_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, xLuaState*>::type extract(NormalTag, Rty(Cty::*func)(xLuaState*))
        {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return func;
        }
    };

    /* �������κβ���
     * ������ȡ�������Ͳ����ĺ���
     * ��������ֺ�������, �����ֱ������
    */
    template <>
    struct Extractor<>
    {
        template <typename Rty, typename Cty, typename... Args>
        static inline auto extract(ConstTag, Rty(Cty::*const_func)(Args...) const) -> typename MemberFuncType<Rty, Cty, Args...>::const_type
        {
            return const_func;
        }

        template <typename Rty, typename Cty, typename... Args>
        static inline auto extract(NormalTag, Rty(Cty::*func)(Args...)) -> typename MemberFuncType<Rty, Cty, Args...>::type
        {
            return func;
        }

        template <typename Rty, typename Cty>
        static inline auto extract(NormalTag, Rty Cty::*var) -> typename MemberDataType<Rty, Cty>::type
        {
            return var;
        }

        template <typename Rty, typename... Args>
        static inline auto extract(AnyTag, Rty(*global_func)(Args...)) -> typename FuncType<Rty, Args...>::type
        {
            return global_func;
        }

        template <typename Ty>
        static inline Ty* extract(AnyTag, Ty* global_var)
        {
            return global_var;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, xLuaState*>::const_type extract(ConstTag, Rty(Cty::*const_func)(xLuaState*) const)
        {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return const_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, xLuaState*>::type extract(NormalTag, Rty(Cty::*func)(xLuaState*))
        {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return func;
        }
    };
} // namespace detail

XLUA_NAMESPACE_END
