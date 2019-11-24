#include "lua_export.h"
#include <xlua_export.h>

namespace Enum {
    using ShapeType = ::ShapeType;
}

XLUA_EXPORT_CONSTANT_BEGIN(ShapeType)
XLUA_CONSTANT(ShapeType::kUnknown)
XLUA_CONSTANT(ShapeType::kTriangle)
XLUA_CONSTANT(ShapeType::kQuard)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CONSTANT_BEGIN(_G1)
XLUA_CONSTANT(ShapeType::kUnknown)
XLUA_CONSTANT(ShapeType::kTriangle)
XLUA_CONSTANT(ShapeType::kQuard)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CONSTANT_BEGIN(Enum::ShapeType)
XLUA_CONSTANT(ShapeType::kUnknown)
XLUA_CONSTANT(ShapeType::kTriangle)
XLUA_CONSTANT(ShapeType::kQuard)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CLASS_BEGIN(ShapeBase)
XLUA_VARIATE_AS(obj_id, ShapeBase::obj_id_)
XLUA_VARIATE_AS(name, ShapeBase::name_)
XLUA_FUNCTION(ShapeBase::AreaSize)
//XLUA_FUNCTION_EXTEND(Ext1, Obj_Ext1)
//XLUA_FUNCTION_EXTEND(Ext2, Obj_Ext2)
//XLUA_FUNCTION_EXTEND(Ext3, Obj_Ext3)
//XLUA_FUNCTION_EXTEND(Ext4, Obj_Ext4)
//XLUA_MEMBER_VAR_EXTEND(Tag1, Obj_GetTag1, Obj_SetTag1)
//XLUA_MEMBER_VAR_EXTEND(Tag2, Obj_GetTag2, Obj_SetTag2)
//XLUA_MEMBER_VAR_EXTEND(Tag3, Obj_GetTag3, Obj_SetTag3)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Triangle, ShapeBase)
XLUA_FUNCTION(Triangle::AreaSize)
XLUA_VARIATE_AS(line_1, Triangle::line_1_)
XLUA_VARIATE_AS(line_2, Triangle::line_2_)
XLUA_VARIATE_AS(line_3, Triangle::line_3_)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Square, Triangle)
XLUA_FUNCTION(Square::Name)
XLUA_FUNCTION(Square::DoExtWork)
XLUA_FUNCTION(Square::TestParam_1)
XLUA_FUNCTION(Square::TestParam_2)
XLUA_FUNCTION(Square::TestParam_3)
XLUA_FUNCTION(Square::TestParam_4)
XLUA_FUNCTION_AS(AddTriangle_1, Square::AddTriangle, Triangle*)
XLUA_FUNCTION_AS(AddTriangle_2, Square::AddTriangle, Triangle&)
XLUA_FUNCTION_AS(AddTriangle_3, Square::AddTriangle, const Triangle&)
XLUA_FUNCTION_AS(AddTriangle_4, Square::AddTriangle, const std::shared_ptr<Triangle>&)
XLUA_VARIATE_AS(type, Square::type_)
XLUA_VARIATE_AS(width, Square::width_)
XLUA_VARIATE_AS(height, Square::height_)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(WeakObj)
//XLUA_VARIATE_AS(index, WeakObj::index_)
//XLUA_FUNCTION(WeakObj::SetArea)
//XLUA_FUNCTION(WeakObj::SetArea2)
XLUA_EXPORT_CLASS_END();

XLUA_EXPORT_CLASS_BEGIN(TestExportParams)
XLUA_FUNCTION_AS(TestBool, TestExportParams::Test, bool)
XLUA_FUNCTION_AS(TestChar, TestExportParams::Test, char)
XLUA_FUNCTION_AS(TestUChar, TestExportParams::Test, unsigned char)
XLUA_FUNCTION_AS(TestShort, TestExportParams::Test, short)
XLUA_FUNCTION_AS(TestUShort, TestExportParams::Test, unsigned short)
XLUA_FUNCTION_AS(TestInt, TestExportParams::Test, int)
XLUA_FUNCTION_AS(TestUInt, TestExportParams::Test, unsigned int)
XLUA_FUNCTION_AS(TestLongLong, TestExportParams::Test, long long)
XLUA_FUNCTION_AS(TestULongLong, TestExportParams::Test, unsigned long long)
XLUA_FUNCTION_AS(TestFloat, TestExportParams::Test, float)
XLUA_FUNCTION_AS(TestDouble, TestExportParams::Test, double)
//XLUA_FUNCTION_AS(TestCharPtr, Test, char*) // compile error
XLUA_FUNCTION_AS(TestConstCharPtr, TestExportParams::Test, const char*)
XLUA_FUNCTION_AS(TestVoidPtr, TestExportParams::Test, void*)
XLUA_FUNCTION_AS(TestConstVoidPtr, TestExportParams::Test, const void*)
XLUA_FUNCTION_AS(TestTrianglePtr, TestExportParams::Test, Triangle*)
XLUA_FUNCTION_AS(TestConstTrianglePtr, TestExportParams::Test, const Triangle*)
XLUA_FUNCTION_AS(TestTriangleRef, TestExportParams::Test, Triangle&)
XLUA_FUNCTION_AS(TestConstTriangleRef, TestExportParams::Test, const Triangle&)
XLUA_FUNCTION_AS(TestTriangleValue, TestExportParams::TestValue)
//XLUA_FUNCTION_AS(TestNoneExport1, TestNoneExport, NoneExport)                  // compile error
//XLUA_FUNCTION_AS(TestNoneExport2, TestNoneExport, NoneExport*)                 // compile error
//XLUA_FUNCTION_AS(TestNoneExport3, TestNoneExport, std::shared_ptr<NoneExport>) // compile error
//XLUA_FUNCTION_AS(TestNoneExport4, TestNoneExport, xLuaWeakObjPtr<NoneExport>)  // compile error
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Global::TestStaticParams)
using cls = Global::TestStaticParams;
XLUA_FUNCTION_AS(TestBool, cls::Test, bool)
XLUA_FUNCTION_AS(TestChar, cls::Test, char)
XLUA_FUNCTION_AS(TestUChar, cls::Test, unsigned char)
XLUA_FUNCTION_AS(TestShort, cls::Test, short)
XLUA_FUNCTION_AS(TestUShort, cls::Test, unsigned short)
XLUA_FUNCTION_AS(TestInt, cls::Test, int)
XLUA_FUNCTION_AS(TestUInt, cls::Test, unsigned int)
XLUA_FUNCTION_AS(TestLongLong, cls::Test, long long)
XLUA_FUNCTION_AS(TestULongLong, cls::Test, unsigned long long)
XLUA_FUNCTION_AS(TestFloat, cls::Test, float)
XLUA_FUNCTION_AS(TestDouble, cls::Test, double)
//XLUA_FUNCTION_AS(TestCharPtr, Test, char*) // compile error
XLUA_FUNCTION_AS(TestConstCharPtr, cls::Test, const char*)
XLUA_FUNCTION_AS(TestVoidPtr, cls::Test, void*)
XLUA_FUNCTION_AS(TestConstVoidPtr, cls::Test, const void*)
XLUA_FUNCTION_AS(TestTrianglePtr, cls::Test, Triangle*)
XLUA_FUNCTION_AS(TestConstTrianglePtr, cls::Test, const Triangle*)
XLUA_FUNCTION_AS(TestTriangleRef, cls::Test, Triangle&)
XLUA_FUNCTION_AS(TestConstTriangleRef, cls::Test, const Triangle&)
XLUA_FUNCTION_AS(TestTriangleValue, cls::TestValue)
//XLUA_FUNCTION_AS(TestNoneExport1, TestNoneExport, NoneExport)                  // compile error
//XLUA_FUNCTION_AS(TestNoneExport2, TestNoneExport, NoneExport*)                 // compile error
//XLUA_FUNCTION_AS(TestNoneExport3, TestNoneExport, std::shared_ptr<NoneExport>) // compile error
//XLUA_FUNCTION_AS(TestNoneExport4, TestNoneExport, xLuaWeakObjPtr<NoneExport>)  // compile error
XLUA_EXPORT_CLASS_END()
//
//void xLuaPush(xlua::State* l, const Color c) {
//    l->NewTable();
//    l->SetField(-1, "a", c.a);
//    l->SetTableField(-1, "r", c.r);
//    l->SetTableField(-1, "g", c.g);
//    l->SetTableField(-1, "b", c.b);
//}
//
//Color xLuaLoad(xlua::State* l, int i, xlua::Identity<Color>) {
//    if (l->GetType(i) != LUA_TTABLE)
//        return Color();
//
//    Color c;
//    c.a = l->GetTableField<int>(i, "a");
//    c.r = l->GetTableField<int>(i, "r");
//    c.g = l->GetTableField<int>(i, "g");
//    c.b = l->GetTableField<int>(i, "b");
//    return c;
//}
//
//void xLuaPush(xlua::State* l, const Vec2 v) {
//    l->NewTable();
//    l->SetTableField(-1, "x", v.x);
//    l->SetTableField(-1, "y", v.y);
//}
//
//Vec2 xLuaLoad(xlua::State* l, int i, xlua::Identity<Vec2>) {
//    if (l->GetType(i) != LUA_TTABLE)
//        return Vec2();
//
//    Vec2 c;
//    c.x = l->GetTableField<float>(i, "x");
//    c.y = l->GetTableField<float>(i, "y");
//    return c;
//}
//
//void xLuaPush(xlua::State* l, const PushVal v) {
//    l->NewTable();
//    l->SetTableField(-1, "x", v.a);
//    l->SetTableField(-1, "y", v.b);
//}
//
static void TestGlobal() {
    printf("TestGlobal\n");
}

static char global_name[64] ={0};

XLUA_EXPORT_GLOBAL_BEGIN(_G)
XLUA_FUNCTION(TestGlobal)
//XLUA_VARIATE(global_name); // can not add var to global table
XLUA_EXPORT_GLOBAL_END()

XLUA_EXPORT_GLOBAL_BEGIN(Global)
XLUA_FUNCTION(TestGlobal)
XLUA_VARIATE(global_name)
XLUA_EXPORT_GLOBAL_END()


XLUA_EXPORT_CONSTANT_BEGIN(_G)
XLUA_CONSTANT_AS(Version, 1)
XLUA_CONSTANT_AS(Name, "xlua")
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CONSTANT_BEGIN(Const)
XLUA_CONSTANT_AS(Version, 1)
XLUA_CONSTANT_AS(Name, "xlua")
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_SCRIPT("TestExportScript", R"V0G0N(
print("XLUA_EXPORT_SCRIPT executed")
function TestExportScript()
    print("TestExportScript called")
end
)V0G0N");

XLUA_EXPORT_CLASS_BEGIN(ExportObj)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(TestMember)
XLUA_VARIATE(TestMember::boolean_val)
XLUA_VARIATE(TestMember::int_val)
XLUA_VARIATE(TestMember::long_val)
XLUA_VARIATE(TestMember::name_val)
XLUA_VARIATE(TestMember::export_val)
//XLUA_VARIATE(TestMember::none_export_va)  // error, not export
XLUA_VARIATE(TestMember::vector_val)
XLUA_VARIATE(TestMember::map_val)
XLUA_VARIATE_R(TestMember::read_only)
XLUA_FUNCTION(TestMember::Test)
XLUA_VARIATE(TestMember::m_lua_name__)
XLUA_VARIATE(TestMember::s_boolean_val)
XLUA_VARIATE(TestMember::s_int_val)
XLUA_VARIATE(TestMember::s_long_val)
XLUA_VARIATE(TestMember::s_name_val)
XLUA_VARIATE(TestMember::s_export_val)
//XLUA_VARIATE(TestMember::s_none_export_va)  // error, not export
XLUA_VARIATE(TestMember::s_vector_val)
XLUA_VARIATE(TestMember::s_map_val)
XLUA_VARIATE(TestMember::s_lua_name__)
XLUA_VARIATE_R(TestMember::s_read_only)
XLUA_FUNCTION(TestMember::sTest)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CONSTANT_BEGIN(WidgetType)
XLUA_CONSTANT(WidgetType::kImage)
XLUA_CONSTANT(WidgetType::kLabel)
XLUA_CONSTANT(WidgetType::kButton)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CLASS_BEGIN(IRenderer)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Widget)
XLUA_FUNCTION(Widget::GetType)
XLUA_FUNCTION(Widget::Render)
XLUA_VARIATE_AS_R(type, Widget::GetType)
XLUA_VARIATE(Widget::parent)
XLUA_VARIATE(Widget::color)
XLUA_VARIATE(Widget::size)
XLUA_VARIATE(Widget::pos)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Image, Widget)
XLUA_VARIATE(Image::texture)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Label, Widget)
XLUA_VARIATE(Label::text)
XLUA_VARIATE(Label::font)
XLUA_VARIATE(Label::font_size)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Button, Widget)
XLUA_VARIATE(Button::label)
XLUA_VARIATE(Button::onclick)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(LifeTime)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Object)
XLUA_VARIATE(Object::id_)
XLUA_VARIATE(Object::name)
XLUA_FUNCTION(Object::Update)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Character, Object)
XLUA_VARIATE(Character::hp)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Doodad, Object)
XLUA_VARIATE(Doodad::drop_id)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Collider)
XLUA_FUNCTION(Collider::IsEnable)
XLUA_FUNCTION(Collider::Eanble)
XLUA_FUNCTION(Collider::OnEnterTrigger)
XLUA_FUNCTION(Collider::OnLeaveTrigger)
XLUA_FUNCTION(Collider::RayCast)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(BoxCollider, Collider)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(CircleCollider, Collider)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(CapsuleCollider, CircleCollider)
XLUA_EXPORT_CLASS_END()
