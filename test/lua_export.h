#pragma once
#include "shape.h"

XLUA_DECLARE_CLASS(ShapeBase);
XLUA_DECLARE_CLASS(Triangle);
XLUA_DECLARE_CLASS(Square);
XLUA_DECLARE_CLASS(WeakObj);
XLUA_DECLARE_CLASS(TestExportParams);
XLUA_DECLARE_CLASS(Global::TestStaticParams);

void xLuaPush(xlua::xLuaState* l, const Color& c);
Color xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Color>);

void xLuaPush(xlua::xLuaState* l, const Vec2& vec);
Vec2 xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Vec2>);

void xLuaPush(xlua::xLuaState* l, const PushVal& vec);
