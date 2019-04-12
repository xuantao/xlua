#include <stdio.h>
#include <xlua.h>
#include "logic.h"
#include "lua_export.h"
#include "test.h"

int main(int argc, char* argvp[]) {
    xlua::Startup(nullptr);
    xlua::xLuaState* l = xlua::Create(nullptr);

    TestPushLoad(l);
    TestExport(l);
    TestGlobal(l);

    l->Release();
    //system("pause");
    return 0;
}
