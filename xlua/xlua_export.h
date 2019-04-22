/* 导出Lua实现
 * 导出类型、常量、脚本
*/
#pragma once
#include "detail/traits.h"
#include "detail/export.h"
#include "detail/extractor.h"

/* 匿名 */
#define _XLUA_ANONYMOUS_2(line) Anonymous##line
#define _XLUA_ANONYMOUS_1(line) _XLUA_ANONYMOUS_2(line)
#define _XLUA_ANONYMOUS         _XLUA_ANONYMOUS_1(__LINE__)

/* 获取基类 */
#define _XLUA_SUPER_CLASS(...)      typename xlua::detail::BaseType<__VA_ARGS__>::type
/* 提取指定版本的成员函数 */
#define _EXTRACT_METHOD(Func, ...)  xlua::detail::Extractor<__VA_ARGS__>::extract(xlua::detail::ConstTag(), Func)

// 导出实现
#define _XLUA_EXPORT_FUNC_(Name, Func, Meta, IsGlobal)                                  \
    static_assert(!is_null_pointer<decltype(Func)>::value,                         \
        "can not export func:"#Name" with null pointer"                                 \
    );                                                                                  \
    struct _XLUA_ANONYMOUS {                                                            \
        static int call(lua_State* l) {                                                 \
            auto* xl = (xlua::xLuaState*)(lua_touserdata(l, lua_upvalueindex(1)));      \
            assert(xl);                                                                 \
            return xlua::detail::Meta<class_type>::Call(xl, s_type_info,                \
                xlua::detail::PerifyMemberName(#Name), Func);                           \
        }                                                                               \
    };                                                                                  \
    desc->AddMember(#Name, &_XLUA_ANONYMOUS::call, IsGlobal);

#define _XLUA_EXPORT_FUNC(Name, Func, Meta)   _XLUA_EXPORT_FUNC_(Name, Func, Meta, !std::is_member_function_pointer<decltype(Func)>::value)

#define _XLUA_EXPORT_VAR_(Name, GetOp, SetOp, Meta, IsGlobal)                           \
    static_assert(                                                                      \
        xlua::detail::IndexerTrait<decltype(GetOp), decltype(SetOp)>::is_allow,         \
        "can not export var:"#Name" to lua"                                             \
    );                                                                                  \
    struct _XLUA_ANONYMOUS {                                                            \
        static void Get(xlua::xLuaState* l, void* obj, const xlua::detail::TypeInfo* info) {    \
            xlua::detail::Meta<class_type>::Get(l, obj, info, s_type_info, GetOp);      \
        }                                                                               \
        static void Set(xlua::xLuaState* l, void* obj, const xlua::detail::TypeInfo* info) {    \
            xlua::detail::Meta<class_type>::Set(l, obj, info, s_type_info,              \
                xlua::detail::PerifyMemberName(#Name), SetOp);                          \
        }                                                                               \
    };                                                                                  \
    desc->AddMember(#Name,                                                              \
        is_null_pointer<decltype(GetOp)>::value ? nullptr : &_XLUA_ANONYMOUS::Get, \
        is_null_pointer<decltype(SetOp)>::value ? nullptr : &_XLUA_ANONYMOUS::Set, \
        IsGlobal                                                                        \
    );

#define _XLUA_IS_STATIC_VAR(GetOp, SetOp)           !xlua::detail::IndexerTrait<decltype(GetOp), decltype(SetOp)>::is_member
#define _XLUA_EXPORT_VAR(Name, GetOp, SetOp, Meta)  _XLUA_EXPORT_VAR_(Name, GetOp, SetOp, Meta, _XLUA_IS_STATIC_VAR(GetOp, SetOp))

/* 导出Lua类开始 */
#define XLUA_EXPORT_CLASS_BEGIN(ClassName)                                              \
    static_assert(std::is_void<ClassName::LuaDeclare::super>::value                     \
        || std::is_base_of<ClassName::LuaDeclare::super, ClassName>::value,             \
        "internal class is not inherited"                                               \
    );                                                                                  \
    static_assert(std::is_void<ClassName::LuaDeclare::super>::value                     \
        || xlua::detail::IsInternal<ClassName::LuaDeclare::super>::value                \
        || xlua::detail::IsExternal<ClassName::LuaDeclare::super>::value,               \
        "base type is not declare to export to lua"                                     \
    );                                                                                  \
    namespace {                                                                         \
        xlua::detail::TypeNode _XLUA_ANONYMOUS(&ClassName::xLuaGetTypeInfo);            \
    } /* end namespace */                                                               \
    const xlua::detail::TypeInfo* ClassName::xLuaGetTypeInfo() {                        \
        using class_type = ClassName;                                                   \
        static const xlua::detail::TypeInfo* s_type_info = nullptr;                     \
        if (s_type_info)                                                                \
            return s_type_info;                                                         \
        auto* global = xlua::detail::GlobalVar::GetInstance();                          \
        if (global == nullptr)                                                          \
            return nullptr;                                                             \
        auto* desc = global->AllocType(xlua::detail::TypeCategory::kInternal,           \
            xlua::detail::IsWeakObjPtr<ClassName>::value, #ClassName,                   \
            xlua::detail::GetTypeInfoImpl<LuaDeclare::super>()                          \
        );                                                                              \
        if (desc == nullptr)                                                            \
            return nullptr;                                                             \
        static xlua::detail::TypeCasterImpl<ClassName, LuaDeclare::super> s_caster;     \
        desc->SetCaster(&s_caster);


/* 导出lua类结束 */
#define XLUA_EXPORT_CLASS_END()                                                         \
        s_type_info = desc->Finalize();                                                 \
        s_caster.info_ = s_type_info;                                                   \
        return s_type_info;                                                             \
    }

/* 导出外部类 */
#define XLUA_EXPORT_EXTERNAL_BEGIN(ClassName, ...)                                      \
    static_assert(std::is_void<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value                   \
        || std::is_base_of<_XLUA_SUPER_CLASS(__VA_ARGS__), ClassName>::value,           \
        "external class is not inherited"                                               \
    );                                                                                  \
    static_assert(std::is_void<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value                   \
        || xlua::detail::IsInternal<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value              \
        || xlua::detail::IsExternal<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value,             \
        "base type is not declare to export to lua"                                     \
    );                                                                                  \
    namespace {                                                                         \
        xlua::detail::TypeNode _XLUA_ANONYMOUS([]() -> const xlua::detail::TypeInfo* {  \
            return xLuaGetTypeInfo(xlua::Identity<ClassName>());                        \
        });                                                                             \
    } /* end namespace */                                                               \
    const xlua::detail::TypeInfo* xLuaGetTypeInfo(xlua::Identity<ClassName>) {          \
        using class_type = ClassName;                                                   \
        using super_type = _XLUA_SUPER_CLASS(__VA_ARGS__);                              \
        static const xlua::detail::TypeInfo* s_type_info = nullptr;                     \
        if (s_type_info)                                                                \
            return s_type_info;                                                         \
        auto* global = xlua::detail::GlobalVar::GetInstance();                          \
        if (global == nullptr)                                                          \
            return nullptr;                                                             \
        auto* desc = global->AllocType(xlua::detail::TypeCategory::kExternal,           \
            xlua::detail::IsWeakObjPtr<ClassName>::value, #ClassName,                   \
            xlua::detail::GetTypeInfoImpl<super_type>()                                 \
        );                                                                              \
        if (desc == nullptr)                                                            \
            return nullptr;                                                             \
        static xlua::detail::TypeCasterImpl<ClassName, super_type> s_caster;            \
        desc->SetCaster(&s_caster);

#define XLUA_EXPORT_EXTERNAL_END()    XLUA_EXPORT_CLASS_END()

/* 导出全局表 */
#define XLUA_EXPORT_GLOBAL_BEGIN(Name)                                                  \
    namespace {                                                                         \
        xlua::detail::TypeNode _XLUA_ANONYMOUS([]() -> const xlua::detail::TypeInfo* {  \
            using class_type = void;                                                    \
            static const xlua::detail::TypeInfo* s_type_info = nullptr;                 \
            if (s_type_info)                                                            \
                return s_type_info;                                                     \
            auto* global = xlua::detail::GlobalVar::GetInstance();                      \
            if (global == nullptr)                                                      \
                return nullptr;                                                         \
            auto* desc = global->AllocType(xlua::detail::TypeCategory::kGlobal,         \
                false, #Name, nullptr);                                                 \
            if (desc == nullptr)                                                        \
                return nullptr;                                                         \

#define XLUA_EXPORT_GLOBAL_END()                                                        \
            s_type_info = desc->Finalize();                                             \
            return s_type_info;                                                         \
        });                                                                             \
    }   /* end namespace*/

/* 成员函数, 支持静态成员函数 */
#define XLUA_MEMBER_FUNC(Func)                  _XLUA_EXPORT_FUNC(Func, _EXTRACT_METHOD(Func), MetaFunc)
#define XLUA_MEMBER_FUNC_AS(Name, Func, ...)    _XLUA_EXPORT_FUNC(Name, _EXTRACT_METHOD(Func, __VA_ARGS__), MetaFunc)
/* 将外部函数包装为成员函数 */
#define XLUA_MEMBER_FUNC_EXTEND(Name, Func)     _XLUA_EXPORT_FUNC_(Name, Func, MetaFuncEx, false)

/* 成员变量, 支持静态成员变量 */
#define XLUA_MEMBER_VAR(Var)                    _XLUA_EXPORT_VAR(Var, Var, Var, MetaVar)
#define XLUA_MEMBER_VAR_AS(Name, Var)           _XLUA_EXPORT_VAR(Name, Var, Var, MetaVar)
#define XLUA_MEMBER_VAR_WRAP(Name, Get, Set)    _XLUA_EXPORT_VAR(Name, Get, Set, MetaVar)
/* 将外部函数导出为成员变量(非静态) */
#define XLUA_MEMBER_VAR_EXTEND(Name, Get, Set)  _XLUA_EXPORT_VAR_(Name, Get, Set, MetaVarEx, false)

/* 全局函数 */
#define XLUA_GLOBAL_FUNC(Func)                  _XLUA_EXPORT_FUNC(Func, Func, MetaFunc)
#define XLUA_GLOBAL_FUNC_AS(Name, Func)         _XLUA_EXPORT_FUNC(Name, Func, MetaFunc)
/* 全局变量 */
#define XLUA_GLOBAL_VAR(Var)                    _XLUA_EXPORT_VAR(Var, Var, Var, MetaVar)
#define XLUA_GLOBAL_VAR_AS(Name, Var)           _XLUA_EXPORT_VAR(Name, Var, Var, MetaVar)
#define XLUA_GLOBAL_VAR_WARP(Name, Get, Set)    _XLUA_EXPORT_VAR(Name, Get, Set, MetaVar)

/* 导出常量表 */
#define XLUA_EXPORT_CONSTANT_BEGIN(Name)                                                \
    namespace {                                                                         \
        xlua::detail::ConstNode _XLUA_ANONYMOUS([]()-> const xlua::detail::ConstInfo* { \
            static xlua::detail::ConstInfo info;                                        \
            info.name = #Name;                                                          \
            static xlua::detail::ConstValue values[] = {

#define XLUA_EXPORT_CONSTANT_END()                                                      \
                xlua::detail::MakeConstValue()                                          \
            };                                                                          \
            info.values = values;                                                       \
            return &info;                                                               \
        }); /* end function */                                                          \
    } /* end namespace*/

#define XLUA_CONST_VAR(Var)             xlua::detail::MakeConstValue(#Var, Var),
#define XLUA_CONST_VAR_AS(Name, Var)    xlua::detail::MakeConstValue(#Name, Var),

/* 导出预设脚本 */
#define XLUA_EXPORT_SCRIPT(Str)                                                         \
    namespace {                                                                         \
        xlua::detail::ScriptNode _XLUA_ANONYMOUS(Str);                                  \
    }

