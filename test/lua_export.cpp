#include "lua_export.h"
#include <xlua_export.h>

XLUA_EXPORT_EXTERNAL_BEGIN(ObjectBase)
XLUA_MEMBER_VAR_AS(obj_id, obj_id_)
XLUA_MEMBER_VAR_AS(name, name_)
XLUA_MEMBER_FUNC(AreaSize)
XLUA_EXPORT_EXTERNAL_END()

XLUA_EXPORT_CLASS_BEGIN(Triangle)
XLUA_MEMBER_FUNC(AreaSize)
XLUA_MEMBER_VAR_AS(line_1, line_1_)
XLUA_MEMBER_VAR_AS(line_2, line_2_)
XLUA_MEMBER_VAR_AS(line_3, line_3_)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Quard)
XLUA_MEMBER_FUNC(Name)
XLUA_MEMBER_FUNC(DoExtWork)
XLUA_MEMBER_VAR_AS(type, type_)
XLUA_MEMBER_VAR_AS(width, width_)
XLUA_MEMBER_VAR_AS(height, height_)
XLUA_EXPORT_CLASS_END()


void xLuaPush(xlua::xLuaState* l, const Color& c) {
    l->NewTable();
    l->SetTableField(-1, 'a', c.a);
    l->SetTableField(-1, 'r', c.r);
    l->SetTableField(-1, 'g', c.g);
    l->SetTableField(-1, 'b', c.b);
}

Color xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Color>) {
    if (l->GetType(i) != LUA_TTABLE)
        return Color();

    Color c;
    c.a = l->GetTableField<int>(i, "a");
    c.r = l->GetTableField<int>(i, "r");
    c.g = l->GetTableField<int>(i, "g");
    c.b = l->GetTableField<int>(i, "b");
    return c;
}

void xLuaPush(xlua::xLuaState* l, const Vec2& v) {
    l->NewTable();
    l->SetTableField(-1, "x", v.x);
    l->SetTableField(-1, "y", v.y);
}

Vec2 xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Vec2>) {
    if (l->GetType(i) != LUA_TTABLE)
        return Vec2();

    Vec2 c;
    c.x = l->GetTableField<float>(i, "x");
    c.y = l->GetTableField<float>(i, "y");
    return c;
}
