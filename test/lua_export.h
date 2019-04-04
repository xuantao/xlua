#pragma once
#include "logic.h"

XLUA_DECLARE_EXTERNAL(ObjectBase);

void xLuaPush(xlua::xLuaState* l, const Color& c);
Color xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Color>);

void xLuaPush(xlua::xLuaState* l, const Vec2& vec);
Vec2 xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Vec2>);
