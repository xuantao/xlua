#include "test.h"
#include "lua_export.h"
#include <xlua.h>
#include <string.h>

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
    local new_base = xlua.Cast(base, "ShapeBase")
    print("CastToTriangle_1", triangle.line_1, triangle.line_2, triangle.line_3)

    new_base.line_1 = 3
    new_base.line_2 = 4
    new_base.line_3 = 5
    print("CastToTriangle_2", triangle.line_1, triangle.line_2, triangle.line_3)
end

function CastToSquare(base)
    local quard = xlua.Cast(base, "Square")
    quard.type = 1001
    print("CastToSquare", quard:Name())
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

function TestExtend(obj)
    obj:Ext1(101, "TestExtend")
    obj:Ext2(101, "TestExtend")
    obj:Ext3(101, "TestExtend")
    obj:Ext4(101, "TestExtend")

    print(obj.Tag1)
    print(obj.Tag2)
    print(obj.Tag3)

    obj.Tag1 = 101
    obj.Tag2 = 102
    obj.Tag3 = 103
end

function TestPrintEnum()
    print("ShapeType = {")
    for k, v in pairs(ShapeType) do
        print(k, v)
    end
    print("}")

    print("Enum.ShapeType = {")
    for k, v in pairs(Enum.ShapeType) do
        print(k, v)
    end
    print("}")

    print("_G = {")
    for k, v in pairs(ShapeType) do
        print(k, _G[k])
    end
    print("}")
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

void TestGlobal(xlua::State* l) {
    l->DoString(sLuaBuf, "test_global");
    {
        xlua::StackGuard guard(l->GetLuaState());
        Triangle triangle;
        Square quard;

        l->Call("PrintTriangle", std::tie(), &triangle);
        LOG_TOP_(l);
        l->Call("print", std::tie(), triangle, &triangle, quard, &quard);
        LOG_TOP_(l);
        l->NewTable();
        //l->SetGlobal("Scene.Info.OrignPos", true, false);
        //l->SetGlobal("Scene.Info.OrignPos", Vec2(), false);
        //l->SetGlobal("Scene.Info.OrignPos.x", 10.f, false);

        l->Call("PrintInfo", std::tie());
        LOG_TOP_(l);
        Vec2 v1;
        v1.x = l->GetGlobal<float>("Scene.Info.OrignPos.x");

        //Vec2 v2 = l->GetGlobal<Vec2>("Scene.Info.OrignPos");

        //printf("v1.x=%f, v2.x=%f, v2.y=%f\n", v1.x, v2.x, v2.y);
    }

    {
        Triangle triangle;
        Square quard;
        LOG_TOP_(l);
        auto print = l->GetGlobal<xlua::Function>("print");
        auto table = l->GetGlobal<xlua::Table>("GlobalVar");
        LOG_TOP_(l);
        print(std::tie(), triangle, &triangle, quard, &quard);
        //l->Call(print, std::tie(), triangle, &triangle, quard, &quard);
        LOG_TOP_(l);
        table.Call("DoTest", std::tie());
        LOG_TOP_(l);
    }

    {
        Triangle triangle;
        Square quard;
        //l->Call("CastToTriangle", std::tie(), static_cast<ShapeBase*>(&triangle));
        //l->Call("CastToSquare", std::tie(), static_cast<ShapeBase*>(&quard));
        //l->Call("TestMultyInherit", std::tie(), &quard);

        l->Push(&quard);
        ShapeBase* base1 = l->Get<ShapeBase*>(-1);
        ShapeBase* base2 = &quard;
        assert(base1 == base2); // 关闭多继承时这里的断言会失败
    }

    {
        xlua::StackGuard guard(l->GetLuaState());
        WeakObj weak_obj;
        //l->Push(weak_obj);
        //l->Push(&weak_obj);
        //l->Push(xLuaWeakObjPtr<WeakObj>(nullptr));
        //l->Load<xLuaWeakObjPtr<WeakObj>>(-2);
        //l->Load<WeakObj*>(-2);
        //l->Load<WeakObj>(-2);
    }

    {
        Triangle triangle_1;
        Square quard_1;
        auto triangle_2 = std::make_shared<Triangle>();

        //printf("test triangle nullptr\n");
        //l->Call("TestTriangleNullptr", std::tie(), &quard_1, nullptr);

        //printf("test triangle ptr\n");
        //l->Call("TestTriangle", std::tie(), &quard_1, &triangle_1);

        //printf("test triangle value\n");
        //l->Call("TestTriangle", std::tie(), &quard_1, triangle_1);

        //printf("test std::shared_ptr<triangle>\n");
        //l->Call("TestTriangle", std::tie(), &quard_1, triangle_2);

        //printf("test export global\n");
        //l->Call("TestExportGlobal", std::tie());
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

        lua_gc(l->GetLuaState(), LUA_GCCOLLECT, 0);
    }
    {
        ShapeBase o;
        Square q;

        //l->Call("TestExtend", std::tie(), &o);
        //l->Call("TestExtend", std::tie(), &q);
        //l->Call("TestPrintEnum", std::tie());
    }

    LOG_TOP_(l);
}
