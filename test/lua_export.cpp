#include "lua_export.h"
#include <xlua_export.h>

static int Obj_Ext1(ShapeBase* obj, int a, const char* name) {
    printf("Obj_Ext1(obj:0x%p, a:%d, name:%s\n", obj, a, name);
    return 0;
}

static void Obj_Ext2(ShapeBase* obj, int a, const char* name) {
    printf("Obj_Ext2(obj:0x%p, a:%d, name:%s\n", obj, a, name);
}

static int Obj_Ext3(ShapeBase* obj, xlua::xLuaState* l) {
    int a = 0;
    const char* name = nullptr;
    l->LoadMul(1, std::tie(a, name));
    printf("Obj_Ext3(obj:0x%p, a:%d, name:%s\n", obj, a, name);
    return 0;
}

static int Obj_Ext4(ShapeBase* obj, lua_State* l) {
    int a = 0;
    const char* name = nullptr;
    a = (int)lua_tonumber(l, 1);
    name = lua_tostring(l, 2);
    printf("Obj_Ext4(obj:0x%p, a:%d, name:%s\n", obj, a, name);
    return 0;
}

static const char* Obj_GetTag1(ShapeBase* obj) {
    printf("Obj_GetTag1(obj:0x%p)\n", obj);
    return "Obj_GetTag1";
}

static int Obj_GetTag2(ShapeBase* obj, xlua::xLuaState* l) {
    printf("Obj_GetTag2(obj:0x%p)\n", obj);
    l->Push("Obj_GetTag2");
    return 1;
}

static int Obj_GetTag3(ShapeBase* obj, lua_State* l) {
    printf("Obj_GetTag3(obj:0x%p)\n", obj);
    lua_pushstring(l, "Obj_GetTag3");
    return 1;
}

static void Obj_SetTag1(ShapeBase* obj, int v) {
    printf("Obj_SetTag1(obj:0x%p), v:%d\n", obj, v);
}

static int Obj_SetTag2(ShapeBase* obj, xlua::xLuaState* l) {
    int v = l->Load<int>(1);
    printf("Obj_SetTag2(obj:0x%p), v:%d\n", obj, v);
    return 0;
}

static int Obj_SetTag3(ShapeBase* obj, lua_State* l) {
    int v = (int)lua_tonumber(l, 1);
    printf("Obj_SetTag3(obj:0x%p), v:%d\n", obj, v);
    return 0;
}

namespace Enum {
    using ShapeType = ::ShapeType;
}

XLUA_EXPORT_CONSTANT_BEGIN(ShapeType)
XLUA_CONST_VAR(ShapeType::kUnknown)
XLUA_CONST_VAR(ShapeType::kTriangle)
XLUA_CONST_VAR(ShapeType::kQuard)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CONSTANT_BEGIN(_G)
XLUA_CONST_VAR(ShapeType::kUnknown)
XLUA_CONST_VAR(ShapeType::kTriangle)
XLUA_CONST_VAR(ShapeType::kQuard)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CONSTANT_BEGIN(Enum::ShapeType)
XLUA_CONST_VAR(ShapeType::kUnknown)
XLUA_CONST_VAR(ShapeType::kTriangle)
XLUA_CONST_VAR(ShapeType::kQuard)
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_EXTERNAL_BEGIN(ShapeBase)
XLUA_MEMBER_VAR_AS(obj_id, &ShapeBase::obj_id_)
XLUA_MEMBER_VAR_AS(name, &ShapeBase::name_)
XLUA_MEMBER_FUNC(&ShapeBase::AreaSize)
XLUA_MEMBER_FUNC_EXTEND(Ext1, &Obj_Ext1)
XLUA_MEMBER_FUNC_EXTEND(Ext2, &Obj_Ext2)
XLUA_MEMBER_FUNC_EXTEND(Ext3, &Obj_Ext3)
XLUA_MEMBER_FUNC_EXTEND(Ext4, &Obj_Ext4)
XLUA_MEMBER_VAR_EXTEND(Tag1, &Obj_GetTag1, &Obj_SetTag1)
XLUA_MEMBER_VAR_EXTEND(Tag2, &Obj_GetTag2, &Obj_SetTag2)
XLUA_MEMBER_VAR_EXTEND(Tag3, &Obj_GetTag3, &Obj_SetTag3)
XLUA_EXPORT_EXTERNAL_END()

XLUA_EXPORT_CLASS_BEGIN(Triangle)
XLUA_MEMBER_FUNC(&Triangle::AreaSize)
XLUA_MEMBER_VAR_AS(line_1, &Triangle::line_1_)
XLUA_MEMBER_VAR_AS(line_2, &Triangle::line_2_)
XLUA_MEMBER_VAR_AS(line_3, &Triangle::line_3_)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(Square)
XLUA_MEMBER_FUNC(&Square::Name)
XLUA_MEMBER_FUNC(&Square::DoExtWork)
XLUA_MEMBER_FUNC(&Square::TestParam_1)
XLUA_MEMBER_FUNC(&Square::TestParam_2)
XLUA_MEMBER_FUNC(&Square::TestParam_3)
XLUA_MEMBER_FUNC(&Square::TestParam_4)
XLUA_MEMBER_FUNC_AS(AddTriangle_1, &Square::AddTriangle, Triangle*)
XLUA_MEMBER_FUNC_AS(AddTriangle_2, &Square::AddTriangle, Triangle&)
XLUA_MEMBER_FUNC_AS(AddTriangle_3, &Square::AddTriangle, const Triangle&)
XLUA_MEMBER_FUNC_AS(AddTriangle_4, &Square::AddTriangle, const std::shared_ptr<Triangle>&)
XLUA_MEMBER_VAR_AS(type, &Square::type_)
XLUA_MEMBER_VAR_AS(width, &Square::width_)
XLUA_MEMBER_VAR_AS(height, &Square::height_)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(WeakObj)
XLUA_MEMBER_VAR_AS(index, &WeakObj::index_)
XLUA_MEMBER_FUNC(&WeakObj::SetArea)
XLUA_MEMBER_FUNC(&WeakObj::SetArea2)
XLUA_EXPORT_CLASS_END();

XLUA_EXPORT_EXTERNAL_BEGIN(TestExportParams)
XLUA_MEMBER_FUNC_AS(TestBool, &TestExportParams::Test, bool)
XLUA_MEMBER_FUNC_AS(TestChar, &TestExportParams::Test, char)
XLUA_MEMBER_FUNC_AS(TestUChar, &TestExportParams::Test, unsigned char)
XLUA_MEMBER_FUNC_AS(TestShort, &TestExportParams::Test, short)
XLUA_MEMBER_FUNC_AS(TestUShort, &TestExportParams::Test, unsigned short)
XLUA_MEMBER_FUNC_AS(TestInt, &TestExportParams::Test, int)
XLUA_MEMBER_FUNC_AS(TestUInt, &TestExportParams::Test, unsigned int)
XLUA_MEMBER_FUNC_AS(TestLongLong, &TestExportParams::Test, long long)
XLUA_MEMBER_FUNC_AS(TestULongLong, &TestExportParams::Test, unsigned long long)
XLUA_MEMBER_FUNC_AS(TestFloat, &TestExportParams::Test, float)
XLUA_MEMBER_FUNC_AS(TestDouble, &TestExportParams::Test, double)
//XLUA_MEMBER_FUNC_AS(TestCharPtr, Test, char*) // compile error
XLUA_MEMBER_FUNC_AS(TestConstCharPtr, &TestExportParams::Test, const char*)
XLUA_MEMBER_FUNC_AS(TestVoidPtr, &TestExportParams::Test, void*)
XLUA_MEMBER_FUNC_AS(TestConstVoidPtr, &TestExportParams::Test, const void*)
XLUA_MEMBER_FUNC_AS(TestTrianglePtr, &TestExportParams::Test, Triangle*)
XLUA_MEMBER_FUNC_AS(TestConstTrianglePtr, &TestExportParams::Test, const Triangle*)
XLUA_MEMBER_FUNC_AS(TestTriangleRef, &TestExportParams::Test, Triangle&)
XLUA_MEMBER_FUNC_AS(TestConstTriangleRef, &TestExportParams::Test, const Triangle&)
XLUA_MEMBER_FUNC_AS(TestTriangleValue, &TestExportParams::TestValue)
//XLUA_MEMBER_FUNC_AS(TestNoneExport1, TestNoneExport, NoneExport)                  // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport2, TestNoneExport, NoneExport*)                 // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport3, TestNoneExport, std::shared_ptr<NoneExport>) // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport4, TestNoneExport, xLuaWeakObjPtr<NoneExport>)  // compile error
XLUA_EXPORT_EXTERNAL_END()

XLUA_EXPORT_EXTERNAL_BEGIN(Global::TestStaticParams)
using cls = Global::TestStaticParams;
XLUA_MEMBER_FUNC_AS(TestBool, &cls::Test, bool)
XLUA_MEMBER_FUNC_AS(TestChar, &cls::Test, char)
XLUA_MEMBER_FUNC_AS(TestUChar, &cls::Test, unsigned char)
XLUA_MEMBER_FUNC_AS(TestShort, &cls::Test, short)
XLUA_MEMBER_FUNC_AS(TestUShort, &cls::Test, unsigned short)
XLUA_MEMBER_FUNC_AS(TestInt, &cls::Test, int)
XLUA_MEMBER_FUNC_AS(TestUInt, &cls::Test, unsigned int)
XLUA_MEMBER_FUNC_AS(TestLongLong, &cls::Test, long long)
XLUA_MEMBER_FUNC_AS(TestULongLong, &cls::Test, unsigned long long)
XLUA_MEMBER_FUNC_AS(TestFloat, &cls::Test, float)
XLUA_MEMBER_FUNC_AS(TestDouble, &cls::Test, double)
//XLUA_MEMBER_FUNC_AS(TestCharPtr, Test, char*) // compile error
XLUA_MEMBER_FUNC_AS(TestConstCharPtr, &cls::Test, const char*)
XLUA_MEMBER_FUNC_AS(TestVoidPtr, &cls::Test, void*)
XLUA_MEMBER_FUNC_AS(TestConstVoidPtr, &cls::Test, const void*)
XLUA_MEMBER_FUNC_AS(TestTrianglePtr, &cls::Test, Triangle*)
XLUA_MEMBER_FUNC_AS(TestConstTrianglePtr, &cls::Test, const Triangle*)
XLUA_MEMBER_FUNC_AS(TestTriangleRef, &cls::Test, Triangle&)
XLUA_MEMBER_FUNC_AS(TestConstTriangleRef, &cls::Test, const Triangle&)
XLUA_MEMBER_FUNC_AS(TestTriangleValue, &cls::TestValue)
//XLUA_MEMBER_FUNC_AS(TestNoneExport1, TestNoneExport, NoneExport)                  // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport2, TestNoneExport, NoneExport*)                 // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport3, TestNoneExport, std::shared_ptr<NoneExport>) // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport4, TestNoneExport, xLuaWeakObjPtr<NoneExport>)  // compile error
XLUA_EXPORT_EXTERNAL_END()

void xLuaPush(xlua::xLuaState* l, const Color& c) {
    l->NewTable();
    l->SetTableField(-1, "a", c.a);
    l->SetTableField(-1, "r", c.r);
    l->SetTableField(-1, "g", c.g);
    l->SetTableField(-1, "b", c.b);
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

void xLuaPush(xlua::xLuaState* l, const PushVal& v) {
    l->NewTable();
    l->SetTableField(-1, "x", v.a);
    l->SetTableField(-1, "y", v.b);
}

static void TestGlobal() {
    printf("TestGlobal\n");
}

static char global_name[64] ={0};

XLUA_EXPORT_GLOBAL_BEGIN(_G)
XLUA_GLOBAL_FUNC(&TestGlobal)
//XLUA_GLOBAL_VAR(global_name); // can not add var to global table
XLUA_EXPORT_GLOBAL_END()

XLUA_EXPORT_GLOBAL_BEGIN(Global)
XLUA_GLOBAL_FUNC(&TestGlobal)
XLUA_GLOBAL_VAR(&global_name)
XLUA_EXPORT_GLOBAL_END()


XLUA_EXPORT_CONSTANT_BEGIN(_G)
XLUA_CONST_VAR_AS(Version, 1)
XLUA_CONST_VAR_AS(Name, "xlua")
XLUA_EXPORT_CONSTANT_END()

XLUA_EXPORT_CONSTANT_BEGIN(Const)
XLUA_CONST_VAR_AS(Version, 1)
XLUA_CONST_VAR_AS(Name, "xlua")
XLUA_EXPORT_CONSTANT_END()

