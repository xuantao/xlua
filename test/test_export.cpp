static const char* s_lua_str = R"V0G0N(
Test = {}
Test.triangle = nil

function TestType(obj)
    print(string.format("TestType obj:%s", xlua.Type(obj)))
    return xlua.Type(obj)
end

function PrintTriangleType()
    print(string.format("Test.triangle:%s IsValid:%s", xlua.Type(Test.triangle), xlua.IsValid(Test.triangle)))
end

function AddExntedMeta()
    local meta = xlua.GetTypeMeta("Triangle")
    meta.LuaExtend = function (triangle)
        triangle.name = "LuaExtend"
        print("Triangle:LuaExtend called")
        return true
    end
end

function TraverseMetas()
    local list = xlua.GetMetaList()
    for _, v in ipairs(list) do
        print(string.format("%s {", v.__name))
        for name, member in pairs(v) do
            print(string.format("    %s: %s", name, type(member)))
        end
        print("}")
    end
end

)V0G0N";

#include "test.h"
#include "lua_export.h"
#include <xlua_state.h>

static void TestGetType(xlua::xLuaState* l) {
    xlua::xLuaGuard guard(l);

    {
        Triangle triangle;
        l->SetGlobalVar("Test.triangle", &triangle, true);

        auto t_0 = l->GetGlobalVar<Triangle*>("Test.triangle");
        assert(t_0 == &triangle);

        l->Call("PrintTriangleType", std::tie());
    }

    auto t_1 = l->GetGlobalVar<Triangle*>("Test.triangle");
    assert(t_1 == nullptr);
    l->Call("PrintTriangleType", std::tie());
}


void TestExport(xlua::xLuaState* l)
{
    xlua::xLuaGuard guard(l);
    l->DoString(s_lua_str, "TestExport");

    l->Call("TraverseMetas", std::tie());
    l->Call("AddExntedMeta", std::tie());
    l->Call("TraverseMetas", std::tie());

    TestGetType(l);

}
