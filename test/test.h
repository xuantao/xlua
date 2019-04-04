#pragma once
#include <xlua_def.h>

#define LOG_TOP_(l) printf("function:%s, line:%d, lua_stack:%d\n", __FUNCTION__, __LINE__, l->GetTopIndex())

void TestGlobal(xlua::xLuaState* l);
