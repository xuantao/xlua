#pragma once
#include "xlua_config.h"
#include "xlua_def.h"
#include "xlua_state.h"

/* 扩展类型
 * void xLuaPush(xlua::xLuaState* l, const Type& val)
 * Type xLuaLoad(xlua::xLuaState* L, int index, xlua::Identity<Type>);
 * bool xLuaIsType(xlua::xLuaState* L, int index, xlua::Identity<Type>);
*/

XLUA_NAMESPACE_BEGIN

bool Startup();
void Shutdown();
xLuaState* Create(const char* export_module);
xLuaState* Attach(lua_State* l, const char* export_module);

XLUA_NAMESPACE_END
