#include "export_prepare.h"
#include "xlua_export.h"
#include "xlua_state.h"

int Derived::sIdx = 0;

const xlua::TypeDesc* xLuaGetTypeDesc(xlua::Identity<TestObj>) {
    static const xlua::TypeDesc* desc = []()->const xlua::TypeDesc* {
        auto* factory = xlua::CreateFactory<TestObj>("TestObj");
        factory->AddMember(false, "test", [](lua_State*l)->int {
            return 0;
        });

        return factory->Finalize();
    }();

    return desc;
}

const xlua::TypeDesc* xLuaGetTypeDesc(xlua::Identity<Derived>) {
    using meta = xlua::internal::Meta<Derived>;
    using xlua::internal::PurifyMemberName;
    using xlua::internal::GetState;

    static const xlua::TypeDesc* desc = []()->const xlua::TypeDesc* {
        auto* factory = xlua::CreateFactory<Derived, TestObj>("Derived");

        factory->AddMember(false, "print", [](lua_State*l)->int {
            return meta::Call(GetState(l), desc, PurifyMemberName("print"), &Derived::print);
        });

        factory->AddMember(false, "szName", [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Get(l, obj, info, desc, &Derived::szName);
        }, [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Set(l, obj, info, desc, PurifyMemberName("szName"), &Derived::szName);
        });

        factory->AddMember(true, "sIdx", [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Get(l, obj, info, desc, &Derived::sIdx);
        }, [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Set(l, obj, info, desc, PurifyMemberName("sIdx"), &Derived::sIdx);
        });

        return factory->Finalize();
    }();

    return desc;
}

/* register to xlua system */
namespace {
    struct TypeNode_1 : xlua::internal::TypeNode {
        void Reg() override {
            ::xLuaGetTypeDesc(xlua::Identity<TestObj>());
        }
    } g1;

    struct TypeNode_2 : xlua::internal::TypeNode {
        void Reg() override {
            ::xLuaGetTypeDesc(xlua::Identity<Derived>());
        }
    } g2;
}

