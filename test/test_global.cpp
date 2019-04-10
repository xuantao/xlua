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

function TestTriangle_1(quard, triangle)
    quard:AddTriangle_1(triangle)
    quard:AddTriangle_2(triangle)
    quard:AddTriangle_3(triangle)
    quard:AddTriangle_4(triangle)
end

function TestTriangle_2(quard, triangle)
    TestTriangle_1(quard, triangle)
end

function TestTriangle(quard, triangle)
    --quard:AddTriangle_1(triangle)
    --quard:AddTriangle_2(triangle)
    --quard:AddTriangle_3(triangle)
    --quard:AddTriangle_4(triangle)
    TestTriangle_2(quard, triangle)
end

function TestTriangleNullptr(quard)
    quard:AddTriangle_1(nil)
    quard:AddTriangle_4(nil)
    quard:AddTriangle_2(nil)
    quard:AddTriangle_3(nil)
end

function TestExportGlobal()
    TestGlobal()
    Global.TestGlobal()
    Global.global_name = "xuantao"
    print("Global.global_name", Global.global_name)
    print("Version", Version)
    print("Name", Name)
    print("Const.Version", Const.Version)
    print("Const.Name", Const.Name)


    print(getmetatable(Const).__name)
    print(getmetatable(Const).__index)
    print(getmetatable(Const).__newindex)

    Const.Name = "xxxx"
    Const.XXxx = "xuantao"
    print("Const.XXxx", Const.XXxx)
    print("Const.__name", Const.__name)
    print("Const.Name", Const.Name)
end

)V0G0N";

struct ObjTest {
    ObjTest() {
        printf("ObjTest Create\n");
    }

    ObjTest(const ObjTest&) {
        printf("ObjTest Copy\n");
    }

    ~ObjTest() {
        printf("ObjTest release\n");
    }
};

void TestGlobal(xlua::xLuaState* l) {
    l->DoString(sLuaBuf, "test_global");
    {
        xlua::xLuaGuard guard(l);
        Triangle triangle;
        Quard quard;

        l->Call("PrintTriangle", std::tie(), &triangle);
        LOG_TOP_(l);
        l->Call("print", std::tie(), triangle, &triangle, quard, &quard);
        LOG_TOP_(l);
        l->NewTable();
        l->SetGlobal("Scene.Info.OrignPos");
        l->SetGlobal("Scene.Info.OrignPos", Vec2());
        l->SetGlobal("Scene.Info.OrignPos.x", 10.f);

        l->Call("PrintInfo", std::tie());
        LOG_TOP_(l);
        Vec2 v1;
        v1.x = l->GetGlobal<float>("Scene.Info.OrignPos.x");

        Vec2 v2 = l->GetGlobal<Vec2>("Scene.Info.OrignPos");

        printf("v1.x=%f, v2.x=%f, v2.y=%f\n", v1.x, v2.x, v2.y);
    }

    {
        Triangle triangle;
        Quard quard;
        LOG_TOP_(l);
        auto print = l->GetGlobal<xlua::xLuaFunction>("print");
        auto table = l->GetGlobal<xlua::xLuaTable>("GlobalVar");
        LOG_TOP_(l);
        l->Call(print, std::tie(), triangle, &triangle, quard, &quard);
        LOG_TOP_(l);
        l->Call(table, "DoTest", std::tie());
        LOG_TOP_(l);
    }

    {
        Triangle triangle;
        Quard quard;
        l->Call("CastToTriangle", std::tie(), static_cast<ObjectBase*>(&triangle));
        l->Call("CastToQuard", std::tie(), static_cast<ObjectBase*>(&quard));
        l->Call("TestMultyInherit", std::tie(), &quard);

        l->Push(&quard);
        ObjectBase* base1 = l->Load<ObjectBase*>(-1);
        ObjectBase* base2 = &quard;
        assert(base1 == base2); // 关闭多继承时这里的断言会失败
    }

    {
        xlua::xLuaGuard guard(l);
        WeakObj weak_obj;
        l->Push(weak_obj);
        l->Push(&weak_obj);
        l->Push(xLuaWeakObjPtr<WeakObj>(nullptr));
        l->Load<xLuaWeakObjPtr<WeakObj>>(-2);
        l->Load<WeakObj*>(-2);
        l->Load<WeakObj>(-2);
    }

    {
        Triangle triangle_1;
        Quard quard_1;
        auto triangle_2 = std::make_shared<Triangle>();

        printf("test triangle nullptr\n");
        l->Call("TestTriangleNullptr", std::tie(), &quard_1, nullptr);

        printf("test triangle ptr\n");
        l->Call("TestTriangle", std::tie(), &quard_1, &triangle_1);

        printf("test triangle value\n");
        l->Call("TestTriangle", std::tie(), &quard_1, triangle_1);

        printf("test std::shared_ptr<triangle>\n");
        l->Call("TestTriangle", std::tie(), &quard_1, triangle_2);

        printf("test export global\n");
        l->Call("TestExportGlobal", std::tie());
    }

    {
        int a = 0;
        void(*f)(void) = []() {
            printf("xuantao a\n");
        };
        l->Push(f);
        l->Call(std::tie());

        ObjTest obj;
        std::function<void()> f2 = [obj] {
            printf("obj test %p\n", &obj);
        };

        l->Push(f2);
        l->Call(std::tie());

        lua_gc(l->GetState(), LUA_GCCOLLECT, 0);
    }

    LOG_TOP_(l);
}
