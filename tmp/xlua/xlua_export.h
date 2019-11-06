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
        const char* str = s->Load<const char*>(1);
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
            *data = Support<typename PurifyType<Ry>::type>::Load(s, 1);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry Ty::*data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(s, obj->*data, std::extent<Ry>::value);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry Ty::*data, std::false_type) {
        if (CheckMetaVar<Ry>(s, 1, desc, name))
            obj->*data = Support<typename PurifyType<Ry>::type>::Load(s, 1);
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
            func(Support<typename PurifyType<Ry>::type>::Load(s, 1));
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
            (obj->*func)(Support<typename PurifyType<Ry>::type>::Load(s, 1));
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

    /* extend member var */
    /*template <typename Ty, typename Ry>
    inline void MetaGet(State* s, Ty* obj, Ry(*func)(Ty*)) {
        s->Push(func(obj));
    }

    template <typename Ty, typename Ry>
    inline void MetaSet(State* s, Ty* obj, void(*func)(Ty*, Ry)) {
        func(obj, s->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Ty>
    inline void MetaGet(State* s, Ty* obj, int(*func)(Ty*, State*)) {
        func(obj, s);
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, int(*func)(Ty*, State*)) {
        func(obj, s);
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, void(*func)(Ty*, State*)) {
        func(obj, s);
    }

    template <typename Ty>
    inline void MetaGet(State* s, Ty* obj, int(*func)(Ty*, lua_State*)) {
        func(obj, s->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, int(*func)(Ty*, lua_State*)) {
        func(obj, s->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* s, Ty* obj, void(*func)(Ty*, lua_State*)) {
        func(obj, s->GetState());
    }*/

    template <typename Ty, class Cy, typename Ry, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, Ry(Cy::*func)(Args...), index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 2, desc, name)) {
            s->Push((obj->*func)(Support<typename PurifyType<Args>::type>::Load(s, 2 + Idxs)...));
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
            s->Push((obj->*func)(Support<typename PurifyType<Args>::type>::Load(s, 2 + Idxs)...));
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
            (obj->*func)(Support<typename PurifyType<Args>::type>::Load(s, 2 + Idxs)...);
        return 0;
    }

    template <typename Ty, class Cy, typename... Args>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(Args...)) {
        return MetaCall(s, obj, desc, name, func, make_index_sequence_t<sizeof...(Args)>());
    }

    template <typename Ty, class Cy, typename... Args, size_t... Idxs>
    inline int MetaCall(State* s, Ty* obj, const TypeDesc* desc, StringView name, void(Cy::*func)(Args...)const, index_sequence<Idxs...>) {
        if (CheckMetaParameters<Args...>(s, 2, desc, name))
            (obj->*func)(Support<typename PurifyType<Args>::type>::Load(s, 2 + Idxs)...);
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
            s->Push(func(Support<typename PurifyType<Args>::type>::Load(s, 1 + Idxs)...));
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
            func(Support<typename PurifyType<Args>::type>::Load(s, 1 + Idxs)...);
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
            return 0;
        }

        template <typename Fy, typename std::enable_if<!IsMember<Fy>::value, int>::type = 0>
        static inline int Set(State* s, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, Fy f) {
            MetaSet(s, dest, name, f);
            return 1;
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
            Ty* obj = s->Load<Ty*>(1);
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
