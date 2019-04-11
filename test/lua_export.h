#pragma once
#include "logic.h"

XLUA_DECLARE_EXTERNAL(ObjectBase);
XLUA_DECLARE_EXTERNAL(TestExportParams);
XLUA_DECLARE_EXTERNAL(Global::TestStaticParams);

void xLuaPush(xlua::xLuaState* l, const Color& c);
Color xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Color>);

void xLuaPush(xlua::xLuaState* l, const Vec2& vec);
Vec2 xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Vec2>);

void xLuaPush(xlua::xLuaState* l, const PushVal& vec);
