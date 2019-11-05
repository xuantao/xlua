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
    inline void MetaSetArray(State* l, Ty* buf, size_t sz) {
        static_assert(std::is_same<char, Ty>::value, "only support char array");
    }

    template <typename Ry>
    inline void MetaSet_(State* l, Ry* data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(l, *data, std::extent<Ry>::value);
    }

    template <typename Ry>
    inline void MetaSet_(State* l, Ry* data, std::false_type) {
        *data = l->Load<typename std::decay<Ry>::type>(1);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(State* l, Ty* obj, Ry Ty::*data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(l, obj->*data, std::extent<Ry>::value);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(State* l, Ty* obj, Ry Ty::*data, std::false_type) {
        obj->*data = l->Load<typename std::decay<Ry>::type>(1);
    }

    template <typename Ry>
    inline void MetaGet(State* l, Ry* data) {
        l->Push(*data);
    }

    template <typename Ry>
    inline void MetaSet(State* l, Ry* data) {
        MetaSet_(l, data, std::is_array<Ry>());
    }

    template <typename Ry>
    inline void MetaGet(State* l, Ry(*func)()) {
        l->Push(func());
    }

    template <typename Ry>
    inline void MetaSet(State* l, void(*func)(Ry)) {
        func(l->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaGet(State* l, Obj* obj, Ry Ty::* data) {
        l->Push(obj->*data);
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaSet(State* l, Obj* obj, Ry Ty::*data) {
        MetaSet_(l, static_cast<Ty*>(obj), data, std::is_array<Ry>());
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaGet(State* l, Obj* obj, Ry(Ty::*func)()) {
        l->Push((obj->*func)());
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaSet(State* l, Obj* obj, void(Ty::*func)(Ry)) {
        (obj->*func)(l->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Ty>
    inline void MetaGet(State* l, Ty* obj, int(Ty::*func)(State*)) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaGet(State* l, Ty* obj, int(Ty::*func)(State*) const) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, int(Ty::*func)(State*)) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, void(Ty::*func)(State*)) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaGet(State* l, Ty* obj, int(Ty::*func)(lua_State*)) {
        (obj->*func)(l->GetState());
    }

    template <typename Ty>
    inline void MetaGet(State* l, Ty* obj, int(Ty::*func)(lua_State*) const) {
        (obj->*func)(l->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, int(Ty::*func)(lua_State*)) {
        (obj->*func)(l->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, void(Ty::*func)(lua_State*)) {
        (obj->*func)(l->GetState());
    }

    /* extend member var */
    template <typename Ty, typename Ry>
    inline void MetaGet(State* l, Ty* obj, Ry(*func)(Ty*)) {
        l->Push(func(obj));
    }

    template <typename Ty, typename Ry>
    inline void MetaSet(State* l, Ty* obj, void(*func)(Ty*, Ry)) {
        func(obj, l->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Ty>
    inline void MetaGet(State* l, Ty* obj, int(*func)(Ty*, State*)) {
        func(obj, l);
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, int(*func)(Ty*, State*)) {
        func(obj, l);
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, void(*func)(Ty*, State*)) {
        func(obj, l);
    }

    template <typename Ty>
    inline void MetaGet(State* l, Ty* obj, int(*func)(Ty*, lua_State*)) {
        func(obj, l->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, int(*func)(Ty*, lua_State*)) {
        func(obj, l->GetState());
    }

    template <typename Ty>
    inline void MetaSet(State* l, Ty* obj, void(*func)(Ty*, lua_State*)) {
        func(obj, l->GetState());
    }

    template <typename Ty>
    struct IsMember {
        static constexpr bool value = std::is_member_pointer<Ty>::value || std::is_member_function_pointer<Ty>::value;
    };

    template <typename Ty>
    struct Meta {
        template <typename Fy, typename std::enable_if<IsMember<Fy>::value, int>::type = 0>
        static inline int Get(xLuaState* l, void* obj, const TypeDesc* src, const TypeDesc* dest, Fy f) {
            MetaGet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
            return 1;
        }

        template <typename Fy, typename std::enable_if<IsMember<Fy>::value, int>::type = 0>
        static inline int Set(xLuaState* l, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, Fy f) {
            MetaSet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
            return 0;
        }

        template <typename Fy, typename std::enable_if<!IsMember<Fy>::value, int>::type = 0>
        static inline void Get(xLuaState* l, void* obj, const TypeDesc* src, const TypeDesc* dest, Fy f) {
            MetaGet(l, f);
        }

        template <typename Fy, typename std::enable_if<!IsMember<Fy>::value, int>::type = 0>
        static inline void Set(xLuaState* l, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, Fy f) {
            MetaSet(l, f);
        }

        static inline void Get(xLuaState* l, void* obj, const TypeDesc* src, const TypeDesc* dest, std::nullptr_t) {
            assert(false);
        }

        static inline void Set(xLuaState* l, void* obj, const TypeDesc* src, const TypeDesc* dest, StringView name, std::nullptr_t) {
            assert(false);
        }

        template <typename Fy, typename std::enable_if<std::is_member_function_pointer<Fy>::value, int>::type = 0>
        static inline int Call(xLuaState* s, const TypeDesc* desc, StringView name, Fy f) {
            Ty* obj = s->Load<Ty*>(1);
            if (obj == nullptr) {
                luaL_error(l->GetLuaState(), "attempt call function[%s.%s] failed, obj is nil\n%s", desc->name, "");
                return 0;
            }

            //LogBufCache<> lb;
            //Ty* obj = static_cast<Ty*>(GetMetaCallObj(l, info));
            //if (obj == nullptr) {
            //    l->GetCallStack(lb);
            //    luaL_error(l->GetState(), "attempt call function[%s:%s] failed, obj is nil\n%s", info->type_name, func_name, lb.Finalize());
            //    return 0;
            //}

            //lua_remove(l->GetState(), 1);
            //if (!param::CheckMetaParam(l, 1, lb, f)) {
            //    l->GetCallStack(lb);
            //    luaL_error(l->GetState(), "attempt call function[%s:%s] failed, parameter is not allow\n%s", info->type_name, func_name, lb.Finalize());
            //    return 0;
            //}

            //return MetaCall(l, obj, f);
            return 0;
        }

        template <typename Fy, typename std::enable_if<!std::is_member_function_pointer<Fy>::value, int>::type = 0>
        static inline int Call(xLuaState* l, const TypeDesc* desc, StringView name, Fy f) {
            //LogBufCache<> lb;
            //if (!param::CheckMetaParam(l, 1, lb, f)) {
            //    l->GetCallStack(lb);
            //    luaL_error(l->GetState(), "attempt call function[%s:%s] failed, parameter is not allow\n%s", info->type_name, func_name, lb.Finalize());
            //    return 0;
            //}

            //return MetaCall(l, f);
            return 0;
        }
    };

    ITypeFactory* CreateFactory(bool global, const char* path, const TypeDesc* super);
} // namespace internal

template <typename Ty = void, typename By = void,
    typename std::enable_if<std::is_void<Ty>::value && std::is_same<Ty, By>::value, int>::type = 0>
inline ITypeFactory* CreateFactory(const char* name) {
    return internal::CreateFactory(true, name, nullptr);
}

template <typename Ty = void, typename By = void,
    typename std::enable_if<!std::is_void<Ty>::value && !std::is_same<Ty, By>::value, int>::type = 0>
inline ITypeFactory* CreateFactory(const char* name) {
    auto* factory = internal::CreateFactory(false, name, ::xLuaGetTypeDesc(Identity<By>()));
    factory->SetWeakProc(::xLuaQueryWeakObjProc(Identity<Ty>()));
    factory->SetCaster(internal::CasterTraits<Ty, By>::Make());
    return factory;
}

XLUA_NAMESPACE_END
