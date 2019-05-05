#pragma once
#include "xlua_config.h"
#include "xlua_def.h"
#include "xlua_state.h"

XLUA_NAMESPACE_BEGIN

bool Startup(LogFunc fn);
void Shutdown();
xLuaState* Create(const char* export_module);
xLuaState* Attach(lua_State* l, const char* export_module);

XLUA_NAMESPACE_END
