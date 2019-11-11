#pragma once
#include "shape.h"
#include <vector>
#include <xlua_state.h>

XLUA_DECLARE_CLASS(ShapeBase);
XLUA_DECLARE_CLASS(Triangle);
XLUA_DECLARE_CLASS(Square);
XLUA_DECLARE_CLASS(WeakObj);
XLUA_DECLARE_CLASS(TestExportParams);
XLUA_DECLARE_CLASS(Global::TestStaticParams);

//void xLuaPush(xlua::State* l, const Color& c);
//Color xLuaLoad(xlua::State* l, int i, xlua::Identity<Color>);
//
//void xLuaPush(xlua::State* l, const Vec2& vec);
//Vec2 xLuaLoad(xlua::State* l, int i, xlua::Identity<Vec2>);
//
//void xLuaPush(xlua::State* l, const PushVal& vec);
//
//template <typename Ty>
//void xLuaPush(xlua::State* l, const std::vector<Ty>& vec)
//{
//    //l->NewTable();
//}
