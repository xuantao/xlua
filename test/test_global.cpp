#include "test.h"
#include "lua_export.h"
#include <xlua.h>

static const char* sLuaBuf = R"V0G0N(
function PrintInfo()
    print(Scene)
    print(Scene.Info)
    print(Scene.Info.OrignPos)
end

GlobalVar = {}
function GlobalVar:DoTest()
    print("GlobalVar", self, GlobalVar);
end

function CastToTriangle(base)
    local triangle = xlua.Cast(base, "Triangle")
    print("CastToTriangle", triangle.line_1, triangle.line_2, triangle.line_3)
end
)V0G0N";

void TestGlobal(xlua::xLuaState* l) {
    Triangle triangle;
    Quard quard;

    l->DoString(sLuaBuf);

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
    }
    LOG_TOP_(l);
}
