#include <stdio.h>
#include "xlua.h"
#include "xlua_export.h"

struct TestType1
{
    int ia;
};

namespace {
    struct extend_member_var {
        static xlua::LuaIndexer Get() {
            return [](xlua::xLuaState* L, void* obj, const xlua::TypeInfo*) {
                extend_member_var* self = nullptr;
                auto f = [self]() {
                    return self->a;
                };

                //xlua::detail::MetaGet(L, &f, &decltype(f)::operator ());
            };
        }

        static xlua::LuaIndexer Set() {
            return nullptr;
        }
        int a;
    };
}

enum TestEnum
{
    kValue1,
    kValue2,
};

enum class TestEnum2
{
    kValue11,
    kValue12,
};

struct TestNone
{};

namespace test_lua {
    struct TestLuaExport
    {
        int ia;
        char name[64]={0};
        static char buf[64];
        void test_call() {}

        void test_enum(TestEnum2) {}
        void test_enum(int) {}
        TestNone GetNone() { return TestNone(); }

        static void Print() { }

        int TestConst() { return 0; }
        int TestConst() const { return 0; }

        int TestLua(xlua::xLuaState* l) { return 0; }
        int TestLua(xlua::xLuaState* l) const { return 0; }
    };

    char TestLuaExport::buf[64] ={0};
}

bool TestExtend(test_lua::TestLuaExport* o) {
    printf("bool TestExtend(test_lua::TestLuaExport* o)\n");
    return false;
}
int TestExtend2(test_lua::TestLuaExport* o, xlua::xLuaState* l) {
    printf("int TestExtend2(test_lua::TestLuaExport* o, xlua::xLuaState* l)\n");
    l->Push("true");
    return 1;
}

void TestExtend3(test_lua::TestLuaExport* o, bool value) {
    printf("int TestExtend2(test_lua::TestLuaExport* o, xlua::xLuaState* l)\n");
    //l->Push("true");
}

XLUA_EXPORT_ENUM_BEGIN(TestEnum2)
XLUA_EXPORT_ENUM_VAR(kValue11)
XLUA_EXPORT_ENUM_VAR_AS(ttt, kValue12)
XLUA_EXPORT_ENUM_END()

XLUA_DECLARE_EXTERNAL_CLASS(test_lua::TestLuaExport);

XLUA_EXPORT_EXTERNAL_CLASS_BEGIN(test_lua::TestLuaExport)
XLUA_EXPORT_MEMBER_FUNC(test_call)
XLUA_EXPORT_MEMBER_FUNC_AS(test_enum_e, test_enum, TestEnum2)
XLUA_EXPORT_MEMBER_FUNC_AS(test_enum_i, test_enum, int)
//XLUA_EXPORT_MEMBER_FUNC(GetNone)
XLUA_EXPORT_MEMBER_FUNC(Print)
XLUA_EXPORT_MEMBER_FUNC(TestConst)
XLUA_EXPORT_MEMBER_FUNC(TestLua)
XLUA_EXPORT_MEMBER_FUNC_EXTEND(TestExtend, TestExtend)
XLUA_EXPORT_MEMBER_FUNC_EXTEND(TestExtend2, TestExtend2)
XLUA_EXPORT_MEMBER_VAR(ia)
XLUA_EXPORT_MEMBER_VAR(name)
XLUA_EXPORT_MEMBER_VAR(buf)
XLUA_EXPORT_MEMBER_VAR_EXTEND_R(GetBool, TestExtend)
XLUA_EXPORT_MEMBER_VAR_EXTEND_W(SetInt, TestExtend3)
XLUA_EXPORT_EXTERNAL_CLASS_END()

XLUA_EXPORT_GLOBAL_BEGIN(Global.Test)
XLUA_EXPORT_GLOBAL_FUNC(TestExtend)
XLUA_EXPORT_GLOBAL_VAR(test_lua::TestLuaExport::buf)
XLUA_EXPORT_GLOBAL_END()

XLUA_EXPORT_SCRIPT("    \
function call_test()    \
    print('test call')  \
end                     \
function call_extend_1(obj)     \
    print(obj:TestExtend())     \
    obj.name = \"xuantao\"      \
    Global.Test.TestExtend(obj) \
    Global.Test.buf = \"world\" \
end                             \
function call_extend_2(obj)     \
    print(obj:TestExtend2())    \
    test_lua.TestLuaExport.buf = \"test_lua.TestLuaExport\" \
end                             \
");

int main()
{
    xlua::Startup();
    xlua::xLuaState* l = xlua::Create(nullptr);

    test_lua::TestLuaExport* ptr = nullptr;
    test_lua::TestLuaExport obj;

    auto ptr1 = std::make_shared<test_lua::TestLuaExport>();

    l->Push(&obj);
    l->Push(obj);
    l->Push(ptr1);

    printf("active1 top:%d\n", l->GetTopIndex());

    ptr = l->Load<test_lua::TestLuaExport*>(1);
    obj = l->Load<test_lua::TestLuaExport>(1);
    auto obj2 = l->Load<test_lua::TestLuaExport>(2);
    auto ptr2 = l->Load<std::shared_ptr<test_lua::TestLuaExport>>(3);

    xlua::xLuaTable tab;
    xlua::xLuaFunction func;
    printf("active2.1 top:%d\n", l->GetTopIndex());

    l->Call(std::tie(), 1, 1.0f, false, "xxx");
    printf("active2.2 top:%d\n", l->GetTopIndex());
    l->Call("Global.lua.xxx", std::tie(), 1, 1.0f, true);
    printf("active2.3 top:%d\n", l->GetTopIndex());
    l->Call(func, std::tie(), 1, 1.0f, true);
    printf("active2.4 top:%d\n", l->GetTopIndex());
    l->Call(tab, "xxxx", std::tie(), 1, 1.0f, true);

    printf("active2 top:%d\n", l->GetTopIndex());

    l->Call("call_test", std::tie());
    l->Call("print", std::tie(), 1, 1.0f, false, "xxx", func, tab);

    printf("active3 top:%d\n", l->GetTopIndex());

    l->Call("call_extend_1", std::tie(), obj);
    l->Call("call_extend_1", std::tie(), &obj);

    l->Call("call_extend_2", std::tie(), obj);
    l->Call("call_extend_2", std::tie(), &obj);

    printf("active4 top:%d\n", l->GetTopIndex());
    return 0;
}
