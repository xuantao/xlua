#include "lua_export.h"
#include "test.h"
#include <stdio.h>
#include <xlua.h>


int main(int argc, char* argvp[]) {
    xlua::Startup(nullptr);
    xlua::xLuaState* l = xlua::Create(nullptr);

    //TestPushLoad(l);
    //TestExport(l);
    //TestGlobal(l);

    Triangle* triangle;
    l->Push(triangle);

    l->Release();
    //system("pause");
    return 0;
}
