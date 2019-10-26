#include "lua_export.h"
#include "test.h"
#include <stdio.h>
#include <xlua.h>


int main(int argc, char* argvp[]) {
    xlua::Startup(nullptr);
    xlua::xLuaState* l = xlua::Create(nullptr);

    l->LoadGlobal("_G");
    assert(LUA_TTABLE == l->GetType(-1));
    l->PopTop(1);

    TestPushLoad(l);
    TestExport(l);
    TestGlobal(l);



    Triangle* triangle = nullptr;
    l->Push(triangle);

    l->Release();
    return 0;
}
