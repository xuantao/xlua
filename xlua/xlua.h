#pragma once
#include "xlua_state.h"

XLUA_NAMESPACE_BEGIN

State* CreateState(const char* mod);
State* AttachState(lua_State* l, const char* mod);

void FreeObjectIndex(ObjectIndex& index);

XLUA_NAMESPACE_END
