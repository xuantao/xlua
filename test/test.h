#pragma once
#include <xlua_def.h>
#include <stdio.h>

#define LOG_TOP_(l) printf("function:%s, line:%d, lua_stack:%d\n", __FUNCTION__, __LINE__, l->GetTop())

void TestPushLoad(xlua::xLuaState* l);
void TestGlobal(xlua::xLuaState* l);
