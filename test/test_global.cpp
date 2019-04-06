#include "test.h"
#include "lua_export.h"
#include <xlua.h>

static const char* sLuaBuf = R"V0G0N(
function PrintInfo()
    print(Scene)
    print(Scene.Info)
    print(Scene.Info.OrignPos)
end

function PrintTriangle(triangle)
    print("PrintTriangle", triangle.line_1, triangle.line_2, triangle.line_3)
end

GlobalVar = {}
function GlobalVar:DoTest()
    print("GlobalVar", self, GlobalVar);
end

function CastToTriangle(base)
    local triangle = xlua.Cast(base, "Triangle")
    local new_base = xlua.Cast(base, "ObjectBase")
    print("CastToTriangle_1", triangle.line_1, triangle.line_2, triangle.line_3)

    new_base.line_1 = 3
    new_base.line_2 = 4
    new_base.line_3 = 5
    print("CastToTriangle_2", triangle.line_1, triangle.line_2, triangle.line_3)
end

function CastToQuard(base)
    local quard = xlua.Cast(base, "Quard")
    quard.type = 1001
    print("CastToQuard", quard:Name())
end

function TestMultyInherit(quard)
    print("TestMultyInherit", quard:AreaSize(), quard.type)
end

)V0G0N";

void TestGlobal(xlua::xLuaState* l) {
    Triangle triangle;
    Quard quard;

    l->DoString(sLuaBuf, "test_global");

    l->Call("PrintTriangle", std::tie(), &triangle);

    l->Call("print", std::tie(), triangle, &triangle, quard, &quard);

    l->NewTable();
    l->SetGlobal("Scene.Info.OrignPos");
    l->SetGlobal("Scene.Info.OrignPos", Vec2());
    l->SetGlobal("Scene.Info.OrignPos.x", 10.f);

    l->Call("PrintInfo", std::tie());

    Vec2 v1;
    v1.x = l->GetGlobal<float>("Scene.Info.OrignPos.x");

    Vec2 v2 = l->GetGlobal<Vec2>("Scene.Info.OrignPos");

    {
        auto print = l->GetGlobal<xlua::xLuaFunction>("print");
        auto table = l->GetGlobal<xlua::xLuaTable>("GlobalVar");
        l->Call(print, std::tie(), triangle, &triangle, quard, &quard);
        l->Call(table, "DoTest", std::tie());
    }

    {
        l->Call("CastToTriangle", std::tie(), static_cast<ObjectBase*>(&triangle));
        l->Call("CastToQuard", std::tie(), static_cast<ObjectBase*>(&quard));
        l->Call("TestMultyInherit", std::tie(), &quard);

        l->Push(&quard);
        ObjectBase* base1 = l->Load<ObjectBase*>(-1);
        ObjectBase* base2 = &quard;
        assert(base1 == base2); // �رն�̳�ʱ����Ķ��Ի�ʧ��
    }
    LOG_TOP_(l);
}