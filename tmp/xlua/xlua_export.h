#pragma once
#include "common.h"

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
