#pragma once
#include "xlua_state.h"

XLUA_NAMESPACE_BEGIN

namespace internal {
    struct ExportNode;
    void AppendNode(ExportNode* node);

    struct ConstValue {
        /* value type */
        enum class Type {
            kNone,
            kNumber,
            kString,
        };

        struct Value {
            Type type;
            const char* name;
            union {
                double number_val;
                const char* string_val;
            };
        };

        const char* name;
        const Value* vals;

        static inline Value Make() {
            Value cv;
            cv.type = Type::kNone;
            cv.name = nullptr;
            cv.string_val = nullptr;
            return cv;
        }

        static inline Value Make(const char* name, double val) {
            Value cv;
            cv.type = Type::kNumber;
            cv.name = name;
            cv.number_val = val;
            return cv;
        }

        static inline Value Make(const char* name, const char* val) {
            Value cv;
            cv.type = Type::kString;
            cv.name = name;
            cv.string_val = val;
            return cv;
        }

        template <typename Ty, typename std::enable_if<std::is_enum<Ty>::value, int>::type = 0>
        static inline Value Make(const char* name, Ty val) {
            Value cv;
            cv.type = Type::kInteger;
            cv.name = name;
            cv.number_val = (int)val;
            return cv;
        }
    };

    struct ScriptValue {
        const char* name;
        const char* script;
    };

    enum class NodeType {
        kConst,
        kScript,
        kType,
    };

    struct ExportNode {
    protected:
        ExportNode(NodeType ty) : type(ty), next(nullptr) {
            AppendNode(this);
        }

    public:
        ExportNode(const ExportNode&) = delete;
        void operator = (const ExportNode&) = delete;

        NodeType type;
        ExportNode* next;
    };

    struct ConstValueNode : ExportNode {
        ConstValueNode(const ConstValue* vls) :
            ExportNode(NodeType::kConst), value(vls) {}

        const ConstValue* value;
    };

    struct ScriptNode : ExportNode {
        ScriptNode(ScriptValue s) :
            ExportNode(NodeType::kScript), script(s) {}

        const ScriptValue script;
    };

    struct TypeNode : ExportNode {
        TypeNode() : ExportNode(NodeType::kType) {}
        virtual void Reg() = 0;
    };

    template <typename Ty>
    struct TypeExportNode : TypeNode {
        void Reg() override {
            ::xLuaGetTypeDesc(Identity<Ty>());
        }
    };

    template <typename Dy, typename By>
    struct CasterTraits {
    private:
        static void* ToSuper(void* obj) {
            return static_cast<By*>((Dy*)obj);
        }

        template <typename U, typename std::enable_if<std::is_polymorphic<U>::value, int>::type = 0>
        static void* ToDerived(void* obj) {
            return dynamic_cast<Dy*>(static_cast<By*>(obj));
        }

        template <typename U, typename std::enable_if<!std::is_polymorphic<U>::value, int>::type = 0>
        static void* ToDerived(void* obj) {
            return nullptr;
        }

    public:
        static inline TypeCaster Make() {
            return TypeCaster{&ToSuper, &ToDerived<By>};
        }
    };

    template <typename Dy>
    struct CasterTraits<Dy, void> {
        static void* ToSuper(void* obj) {
            return obj;
        }

        static void* ToDerived(void* obj) {
            return obj;
        }

    public:
        static inline TypeCaster Make() {
            return TypeCaster{&ToSuper, &ToDerived};
        }
    };

    template <typename Ty>
    bool CheckMetaVar(State* s, int index, const TypeDesc* desc, StringView name) {
        if (DoCheckParam<Ty>(s, index))
            return true;

        char buff[1024];
        luaL_error(s->GetLuaState(), "attemp to set var[%s.%s] failed, paramenter is not accpeted,\nparams{%s}",
            desc->name, StringCache<>(name).Str(), GetParameterNames<Ty>(buff, 1024, s, index));
        return false;
    }

    template <typename... Args>
    bool CheckMetaParameters(State* s, int index, const TypeDesc* desc, StringView name) {
        if (CheckParameters<Args...>(s, index))
            return true;

        char buff[1024];
        luaL_error(s->GetLuaState(), "attemp to call fcuntion[%s.%s] failed, paramenter is not accpeted,\nparams{%s}",
            desc->name, StringCache<>(name).Str(), GetParameterNames<Args...>(buff, 1024, s, index));
        return false;
    }

    /* set array value, only support string */
    inline void MetaSetArray(State* s, char* buf, size_t sz) {
        const char* str = s->Get<const char*>(1);
        if (str)
            snprintf(buf, sz, str);
        else
            buf[0] = 0;
    }

    /* set array value */
    template <typename Ty>
    inline void MetaSetArray(State* s, Ty* buf, size_t sz) {
        static_assert(std::is_same<char, Ty>::value, "only support char array");
    }

    template <typename Ry>
    inline void MetaSet_(State* s, Ry* data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(s, *data, std::extent<Ry>::value);
    }

    template <typename Ry>
    inline void MetaSet_(State* s, const TypeDesc* desc, StringView name, Ry* data, std::false_type) {
        if (CheckMetaVar<Ry>(s, 1, desc, name))
            *data = SupportTraits<Ry>::supporter::Load(s, 1);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry Ty::*data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(s, obj->*data, std::extent<Ry>::value);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry Ty::*data, std::false_type) {
        if (CheckMetaVar<Ry>(s, 1, desc, name))
            obj->*data = SupportTraits<Ry>::supporter::Load(s, 1);
    }

    template <typename Ry>
    inline void MetaGet(State* s, Ry* data) {
        s->Push(*data);
    }

    template <typename Ry>
    inline void MetaSet(State* s, const TypeDesc* desc, StringView name, Ry* data) {
        MetaSet_(s, desc, name, data, std::is_array<Ry>());
    }

    template <typename Ry>
    inline void MetaGet(State* s, Ry(*func)()) {
        s->Push(func());
    }

    template <typename Ry>
    inline void MetaSet(State* s, const TypeDesc* desc, StringView name, void(*func)(Ry)) {
        if (CheckMetaVar<Ry>(s, 1, desc, name))
            func(SupportTraits<Ry>::supporter::Load(s, 1));
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaGet(State* s, Obj* obj, Ry Ty::* data) {
        s->Push(obj->*data);
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaSet(State* s, Obj* obj, const TypeDesc* desc, StringView name, Ry Ty::*data) {
        MetaSet_(s, static_cast<Ty*>(obj), desc, name, data, std::is_array<Ry>());
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaGet(State* s, Obj* obj, Ry(Ty::*func)()) {
        s->Push((obj->*func)());
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaSet(State* s, Obj* obj, const TypeDesc* desc, StringView name, void(Ty::*func)(Ry)) {
        if (CheckMetaVar<Ry>(s, 1, desc, name))
            (obj->*func)(SupportTraits<Ry>::supporter::Load(s, 1));
    }

    template <typename Ty>
    inline void MetaGet(State* s, Ty* obj, int(Ty::*func)(State*)) {
        (obj->*func)(s);
    }

    template <typename Ty>
    inline void MetaGet(State* s, Ty* obj, int(Ty::*func)(State*) const) {
        (obj->*func)(s);
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, const TypeDesc* desc, StringView name, int(Ty::*func)(State*)) {
        (obj->*func)(s);
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Ty::*func)(State*)) {
        (obj->*func)(s);
    }

    template <typename Ty>
    inline void MetaGet(State* s, Ty* obj, int(Ty::*func)(lua_State*)) {
        (obj->*func)(s->GetState());
    }

    template <typename Ty>
    inline void MetaGet(State* s, Ty* obj, int(Ty::*func)(lua_State*) const) {
        (obj->*func)(s->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, const TypeDesc* desc, StringView name, int(Ty::*func)(lua_State*)) {
        (obj->*func)(s->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Ty::*func)(lua_State*)) {
        (obj->*func)(s->GetState());
    }

    template <typename Ty, class Cy, typename Ry, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry(Cy::*func)(Args...), index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 2, desc, name)) {
            s->Push((obj->*func)(SupportTraits<Args>::supporter::Load(s, 2 + Idxs)...));
            return 1;
        }
        return 0;
    }

    template <typename Ty, class Cy, typename Ry, typename... Args>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry(Cy::*func)(Args...)) {
        return MetaCall(s, obj, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ty, class Cy, typename Ry, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry(Cy::*func)(Args...)const, index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 2, desc, name)) {
            s->Push((obj->*func)(SupportTraits<Args>::supporter::Load(s, 2 + Idxs)...));
            return 1;
        }
        return 0;
    }

    template <typename Ty, class Cy, typename Ry, typename... Args>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry(Cy::*func)(Args...)const) {
        return MetaCall(s, obj, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ty, class Cy, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(Args...), index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 2, desc, name))
            (obj->*func)(SupportTraits<Args>::supporter::Load(s, 2 + Idxs)...);
        return 0;
    }

    template <typename Ty, class Cy, typename... Args>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(Args...)) {
        return MetaCall(s, obj, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ty, class Cy, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(Args...)const, index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 2, desc, name))
            (obj->*func)(SupportTraits<Args>::supporter::Load(s, 2 + Idxs)...);
        return 0;
    }

    template <typename Ty, class Cy, typename... Args>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(Args...)const) {
        return MetaCall(s, obj, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, int(Cy::*func)(State*)) {
        return (obj->*func)(s);
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, int(Cy::*func)(State*)const) {
        return (obj->*func)(s);
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(State*)) {
        (obj->*func)(s);
        return 0;
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(State*)const) {
        (obj->*func)(s);
        return 0;
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, int(Cy::*func)(lua_State*)) {
        return (obj->*func)(s->GetLuaState());
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, int(Cy::*func)(lua_State*)const) {
        return (obj->*func)(s->GetLuaState());
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(lua_State*)) {
        (obj->*func)(s->GetLuaState());
        return 0;
    }

    template <typename Ty, class Cy>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(lua_State*)const) {
        (obj->*func)(s->GetLuaState());
        return 0;
    }

    template <typename Ry, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, Ry(*func)(Args...), index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 1, desc, name)) {
            s->Push(func(SupportTraits<Args>::supporter::Load(s, 1 + Idxs)...));
            return 1;
        }
        return 0;
    }

    template <typename Ry, typename... Args>
    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, Ry(*func)(Args...)) {
        return MetaCall(s, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, void(*func)(Args...), index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 1, desc, name))
            func(SupportTraits<Args>::supporter::Load(s, 1 + Idxs)...);
        return 0;
    }

    template <typename... Args>
    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, void(*func)(Args...)) {
        return MetaCall(s, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, int(*func)(State*)) {
        return func(s);
    }

    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, void(*func)(State*)) {
        func(s);
        return 0;
    }

    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, int(*func)(lua_State*)) {
        return func(s->GetLuaState());
    }

    inline int MetaCall(State* s, const TypeDesc* desc, StringView name, void(*func)(lua_State*)) {
        func(s->GetLuaState());
        return 0;
    }

    template <typename Ty>
    struct IsMember {
        static constexpr bool value = std::is_member_pointer<Ty>::value || std::is_member_function_pointer<Ty>::value;
    };

    template <typename Ty>
    struct Meta {
        template <typename Fy, typename std::enable_if<IsMember<Fy>::value, int>::type = 0>
        static inline int Get(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, Fy f) {
            MetaGet(s, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(obj, src, dest)), f);
            return 1;
        }

        template <typename Fy, typename std::enable_if<IsMember<Fy>::value, int>::type = 0>
        static inline int Set(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, Fy f) {
            MetaSet(s, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(obj, src, dest)), dest, name, f);
            return 0;
        }

        template <typename Fy, typename std::enable_if<!IsMember<Fy>::value, int>::type = 0>
        static inline int Get(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, Fy f) {
            MetaGet(s, f);
            return 1;
        }

        template <typename Fy, typename std::enable_if<!IsMember<Fy>::value, int>::type = 0>
        static inline int Set(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, Fy f) {
            MetaSet(s, dest, name, f);
            return 0;
        }

        static inline void Get(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, std::nullptr_t) {
            assert(false);
            return 0;
        }

        static inline void Set(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, std::nullptr_t) {
            assert(false);
            return 0;
        }

        template <typename Fy, typename std::enable_if<std::is_member_function_pointer<Fy>::value, int>::type = 0>
        static inline int Call(State* s, const TypeDesc* desc, StringView name, Fy f) {
            Ty* obj = s->Get<Ty*>(1);
            if (obj == nullptr) {
                luaL_error(s->GetLuaState(), "attempt call function[%s.%s] failed, obj is nil\n%s", desc->name, "");
                return 0;
            }

            return MetaCall(s, obj, desc, name, f);
        }

        template <typename Fy, typename std::enable_if<!std::is_member_function_pointer<Fy>::value, int>::type = 0>
        static inline int Call(State* s, const TypeDesc* desc, StringView name, Fy f) {
            return MetaCall(s, desc, name, f);
        }
    };

    template <typename... Tys> struct BaseType { static_assert(sizeof...(Tys) > 1, "not allow multy inherit"); };
    template <> struct BaseType<> { typedef void type; };
    template <typename Ty> struct BaseType<Ty> { typedef Ty type; };

    struct conv_any_tag;
    struct conv_normal_tag;
    struct conv_const_tag;

    struct conv_any_tag {
        friend struct conv_normal_tag;
        friend struct conv_const_tag;

    private:
        conv_any_tag() = default;
    };

    struct conv_normal_tag {
        conv_normal_tag() = default;
        conv_normal_tag(const conv_const_tag&) {}

        operator conv_any_tag() const { return conv_any_tag(); }
    };

    struct conv_const_tag {
        conv_const_tag() = default;
        conv_const_tag(const conv_normal_tag&) {}

        operator conv_any_tag() const { return conv_any_tag(); }
    };

    template <typename Rty, typename Cty, typename... Args>
    struct MemberFuncType {
        typedef Rty(Cty::*type)(Args...);
        typedef Rty(Cty::*const_type)(Args...) const;
    };

    template <typename Rty, typename Cty>
    struct MemberDataType {
        typedef Rty Cty::* type;
    };

    template <typename Rty, typename... Args>
    struct FuncType {
        typedef Rty(*type)(Args...);
    };

    template <typename... Args>
    struct Extractor {
        template <typename Cty, typename Rty>
        static inline auto extract(conv_const_tag, Rty(Cty::*const_func)(Args...) const) -> typename MemberFuncType<Rty, Cty, Args...>::const_type {
            return const_func;
        }

        template <typename Cty, typename Rty>
        static inline auto extract(conv_normal_tag, Rty(Cty::*func)(Args...)) -> typename MemberFuncType<Rty, Cty, Args...>::type {
            return func;
        }

        template <typename Rty>
        static inline auto extract(conv_any_tag, Rty(*global_func)(Args...)) -> typename FuncType<Rty, Args...>::type {
            return global_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, State*>::const_type extract(conv_const_tag, Rty(Cty::*const_func)(State*) const) {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return const_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, State*>::type extract(conv_normal_tag, Rty(Cty::*func)(State*)) {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return func;
        }
    };

    template <>
    struct Extractor<void> {
        template <typename Cty, typename Rty>
        static inline auto extract(conv_const_tag, Rty(Cty::*const_func)(void) const) -> typename MemberFuncType<Rty, Cty>::const_type {
            return const_func;
        }

        template <typename Cty, typename Rty>
        static inline auto extract(conv_normal_tag, Rty(Cty::*func)(void)) -> typename MemberFuncType<Rty, Cty>::type {
            return func;
        }

        template <typename Rty>
        static inline auto extract(conv_any_tag, Rty(*global_func)(void)) -> typename FuncType<Rty>::type {
            return global_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, State*>::const_type extract(conv_const_tag, Rty(Cty::*const_func)(State*) const) {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return const_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, State*>::type extract(conv_normal_tag, Rty(Cty::*func)(State*)) {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return func;
        }
    };

    template <>
    struct Extractor<> {
        template <typename Rty, typename Cty, typename... Args>
        static inline auto extract(conv_const_tag, Rty(Cty::*const_func)(Args...) const) -> typename MemberFuncType<Rty, Cty, Args...>::const_type {
            return const_func;
        }

        template <typename Rty, typename Cty, typename... Args>
        static inline auto extract(conv_normal_tag, Rty(Cty::*func)(Args...)) -> typename MemberFuncType<Rty, Cty, Args...>::type {
            return func;
        }

        template <typename Rty, typename Cty>
        static inline auto extract(conv_normal_tag, Rty Cty::*var) -> typename MemberDataType<Rty, Cty>::type {
            return var;
        }

        template <typename Rty, typename... Args>
        static inline auto extract(conv_any_tag, Rty(*global_func)(Args...)) -> typename FuncType<Rty, Args...>::type {
            return global_func;
        }

        template <typename Ty>
        static inline Ty* extract(conv_any_tag, Ty* global_var) {
            return global_var;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, State*>::const_type extract(conv_const_tag, Rty(Cty::*const_func)(State*) const) {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return const_func;
        }

        template <typename Rty, typename Cty>
        static inline typename MemberFuncType<Rty, Cty, State*>::type extract(conv_normal_tag, Rty(Cty::*func)(State*)) {
            static_assert(std::is_same<Rty, int>::value, "lua function return type must be \"int\"");
            return func;
        }
    };

    ITypeFactory* CreateFactory(bool global, const char* path, const TypeDesc* super);
} // namespace internal

/* create global module factory */
template <typename Ty = void, typename By = void,
    typename std::enable_if<std::is_void<Ty>::value && std::is_same<Ty, By>::value, int>::type = 0>
inline ITypeFactory* CreateFactory(const char* name) {
    return internal::CreateFactory(true, name, nullptr);
}

/* create type module factory */
template <typename Ty = void, typename By = void,
    typename std::enable_if<!std::is_void<Ty>::value && !std::is_same<Ty, By>::value, int>::type = 0>
inline ITypeFactory* CreateFactory(const char* name) {
    auto* factory = internal::CreateFactory(false, name, xLuaGetTypeDesc(Identity<By>()));
    factory->SetWeakProc(xLuaQueryWeakObjProc(Identity<Ty>()));
    factory->SetCaster(internal::CasterTraits<Ty, By>::Make());
    return factory;
}

XLUA_NAMESPACE_END

/* 匿名 */
#define _XLUA_ANONYMOUS_2(line) Anonymous##line
#define _XLUA_ANONYMOUS_1(line) _XLUA_ANONYMOUS_2(line)
#define _XLUA_ANONYMOUS         _XLUA_ANONYMOUS_1(__LINE__)

/* 获取基类 */
#define _XLUA_SUPER_CLASS(...)      typename xlua::internal::BaseType<__VA_ARGS__>::type
/* 提取指定版本的成员函数 */
#define _EXTRACT_METHOD(Func, ...)  xlua::internal::Extractor<__VA_ARGS__>::extract(xlua::internal::conv_const_tag(), Func)

// 导出实现
#define _XLUA_EXPORT_FUNC_(Name, Func, Meta, IsGlobal)                                  \
    static_assert(!xlua::detail::is_null_pointer<decltype(Func)>::value,                \
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
        xlua::detail::is_null_pointer<decltype(GetOp)>::value ? nullptr : &_XLUA_ANONYMOUS::Get,\
        xlua::detail::is_null_pointer<decltype(SetOp)>::value ? nullptr : &_XLUA_ANONYMOUS::Set,\
        IsGlobal                                                                        \
    );

#define _XLUA_IS_STATIC_VAR(GetOp, SetOp)           !xlua::detail::IndexerTrait<decltype(GetOp), decltype(SetOp)>::is_member
#define _XLUA_EXPORT_VAR(Name, GetOp, SetOp, Meta)  _XLUA_EXPORT_VAR_(Name, GetOp, SetOp, Meta, _XLUA_IS_STATIC_VAR(GetOp, SetOp))

/* 导出Lua类开始 */
#define XLUA_EXPORT_CLASS_BEGIN(ClassName, ...)                                         \
    static_assert(std::is_void<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value ||                \
        std::is_base_of<_XLUA_SUPER_CLASS(__VA_ARGS__), ClassName>::value,              \
        "external class is not inherited"                                               \
    );                                                                                  \
    static_assert(std::is_void<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value ||                \
        xlua::detail::IsLuaType<_XLUA_SUPER_CLASS(__VA_ARGS__)>::value,                 \
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
        auto* desc = global->AllocType(xlua::detail::TypeCategory::kClass, #ClassName,  \
            xlua::detail::HasObjIndex<ClassName>::value,                                \
            xlua::detail::IsWeakObjPtr<ClassName>::value,                               \
            xlua::detail::GetTypeInfoImpl<super_type>()                                 \
        );                                                                              \
        if (desc == nullptr)                                                            \
            return nullptr;                                                             \
        static xlua::detail::TypeCasterImpl<ClassName, super_type> s_caster;            \
        desc->SetCaster(&s_caster);

/* 导出lua类结束 */
#define XLUA_EXPORT_CLASS_END()                                                         \
        s_type_info = desc->Finalize();                                                 \
        s_caster.info_ = s_type_info;                                                   \
        return s_type_info;                                                             \
    }

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
            auto* desc = global->AllocType(xlua::detail::TypeCategory::kGlobal, #Name,  \
                false, false, nullptr);                                                 \
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
#define XLUA_EXPORT_SCRIPT(Name, Str)                                                   \
    namespace {                                                                         \
        xlua::detail::ScriptNode _XLUA_ANONYMOUS(Name, Str);                            \
    }
