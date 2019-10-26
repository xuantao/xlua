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

        const char* name;
        const Value* vals;
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
        virtual const TypeDesc* GetDesc() = 0;
    };

    template <typename Ty>
    struct TypeExportNode : TypeNode {
        const TypeDesc* GetDesc() override {
            return ::xLuaGetTypeDesc(Identity<Ty>());
        }
    };

}

XLUA_NAMESPACE_END
