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
    static const xlua::TypeDesc* desc = []()->const xlua::TypeDesc* {
        auto* factory = xlua::CreateFactory<Derived, TestObj>("Derived");

        factory->AddMember(false, "print", [](lua_State*l)->int {
            return 0;
        });

        factory->AddMember(false, "szName", [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            xlua::internal::MetaGet(l, (Derived*)_XLUA_TO_SUPER_PTR(obj, info, desc), &Derived::szName);
            return 0;
        }, [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            xlua::internal::MetaSet(l, (Derived*)_XLUA_TO_SUPER_PTR(obj, info, desc), &Derived::szName);
            return 1;
        });

        factory->AddMember(true, "sIdx", [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            xlua::internal::MetaGet(l, &Derived::sIdx);
            //xlua::internal::MetaGet(l, (Derived*)_XLUA_TO_SUPER_PTR(obj, info, desc), &Derived::sIdx);
            return 0;
        }, [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            xlua::internal::MetaSet(l, &Derived::sIdx);
            return 1;
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

