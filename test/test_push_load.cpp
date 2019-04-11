#include "test.h"
#include "lua_export.h"
#include <xlua_state.h>

static int sLuaCall_1(xlua::xLuaState* l) {
    printf("int sLuaCall_1(xlua::xLuaState* l)\n");
    auto f = l->GetTableField<lua_CFunction>(1, "Call_1");
    // 非标准Lua导出函数会通过中转函数转发
    assert((void*)f != &sLuaCall_1);
    return 0;
}

static int sLuaCall_2(lua_State* l) {
    printf("int sLuaCall_2(lua_State* l)\n");

    lua_getfield(l, 1, "Call_2");
    auto f = lua_tocfunction(l, -1);
    assert((void*)f == &sLuaCall_2);
    return 0;
}

static int sLuaCall_3(xlua::xLuaState* l) {
    printf("int sLuaCall_3(xlua::xLuaState* l)\n");
    return 0;
}

static void sTestGlobal(xlua::xLuaState* l) {
    bool is_ok = false;
    l->NewTable();
    // set value and pop the value
    is_ok = l->SetGlobal("Test.Table.Instance", false);
    assert(is_ok == false);
    assert(l->GetTop() == 0);

    l->Push(1);
    is_ok = l->SetGlobal(nullptr, false);
    assert(is_ok == false);
    assert(l->GetTop() == 0);

    // create global table
    l->NewTable();
    is_ok = l->SetGlobal("Test.Table.Instance", true);
    assert(is_ok);
    assert(l->GetTop() == 0);

    // remove global tables
    is_ok = l->SetGlobalVar("Test.Table.Instance", nullptr);
    assert(is_ok);
    is_ok = l->SetGlobalVar("Test.Table", nullptr);
    assert(is_ok);
    is_ok = l->SetGlobalVar("Test", nullptr);
    assert(is_ok);
    assert(l->GetTop() == 0);

    int ty = 0;
    // load value on the stack, is failed push nil on the stack
    ty = l->LoadGlobal("");
    assert(ty == LUA_TNIL);
    assert(l->GetType(-1) == LUA_TNIL);
    assert(l->GetTop() == 1);
    l->PopTop(1);

    ty = l->LoadGlobal("Test");
    assert(ty == LUA_TNIL);
    assert(l->GetType(-1) == LUA_TNIL);
    assert(l->GetTop() == 1);
    l->PopTop(1);

    ty = l->LoadGlobal("Test.Table");
    assert(ty == LUA_TNIL);
    assert(l->GetType(-1) == LUA_TNIL);
    assert(l->GetTop() == 1);
    l->PopTop(1);

    // set global var failed
    is_ok = l->SetGlobalVar("Test.Table.Instance", 1, false);
    assert(is_ok == false);
    assert(l->GetTop() == 0);

    // create table & set global table
    l->SetGlobalVar("Test.Table.Instance", l->CreateTable(), true);
    xlua::xLuaTable table = l->GetGlobalVar<xlua::xLuaTable>("Test.Table.Instance");
    assert(table);
    assert(l->GetTop() == 0);

    is_ok = l->SetTableField(table, "Call_1", &sLuaCall_1);
    assert(is_ok);
    assert(l->GetTop() == 0);

    is_ok = l->SetTableField(table, "Call_2", &sLuaCall_2);
    assert(is_ok);
    assert(l->GetTop() == 0);

    is_ok = l->SetTableField(table, "Call_3", &sLuaCall_3);
    assert(is_ok);
    assert(l->GetTop() == 0);

    if (!l->Call(table, "Call_1", std::tie()))
        assert(false);
    assert(l->GetTop() == 0);

    if (!l->Call(table, "Call_2", std::tie()))
        assert(false);
    assert(l->GetTop() == 0);

    if (!l->Call(table, "Call_3", std::tie()))
        assert(false);
    assert(l->GetTop() == 0);

    xlua::xLuaFunction f = l->GetTableField<xlua::xLuaFunction>(table, "Call_2");
    assert(f);
    assert(l->GetTop() == 0);

    if (!l->Call(f, std::tie(), table))
        assert(false);
}

#define _TEST_PUSH_LOAD_BASIC(Var)  \
    l->Push(Var);   \
    assert(Var == l->Load<decltype(Var)>(-1))

static void sPushLoadBasic(xlua::xLuaState* l) {
    xlua::xLuaGuard guard(l);
    bool b = true;
    char c = 'a';
    unsigned char uc = 'b';
    short s = 101;
    unsigned short us = 102;
    int i = 1001;
    unsigned int ui = 1002;
    long ln = 10001;
    unsigned long ul = 10002;
    long long ll = 10000001;
    unsigned long long ull = 10000002;
    float f = 1.11f;
    double d = 1.111f;
    const char* p = "xlua";

    _TEST_PUSH_LOAD_BASIC(b);
    _TEST_PUSH_LOAD_BASIC(c);
    _TEST_PUSH_LOAD_BASIC(uc);
    _TEST_PUSH_LOAD_BASIC(s);
    _TEST_PUSH_LOAD_BASIC(us);
    _TEST_PUSH_LOAD_BASIC(i);
    _TEST_PUSH_LOAD_BASIC(ui);
    _TEST_PUSH_LOAD_BASIC(ln);
    _TEST_PUSH_LOAD_BASIC(ul);
    _TEST_PUSH_LOAD_BASIC(ll);
    _TEST_PUSH_LOAD_BASIC(ull);
    _TEST_PUSH_LOAD_BASIC(f);
    _TEST_PUSH_LOAD_BASIC(d);

    l->Push(p);
    assert(strcmp(p, l->Load<const char*>(-1)) == 0);

    l->Push((const void*)p);
    assert(l->Load<const void*>(-1) == p);

    assert(l->GetTop() == 15);
}

void TestPushLoad(xlua::xLuaState* l)
{
    int top = l->GetTop();

    sPushLoadBasic(l);
    sTestGlobal(l);

    assert(top == l->GetTop());
}
