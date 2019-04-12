#include "lua_export.h"
#include <xlua_export.h>

static int Obj_Ext1(ObjectBase* obj, int a, const char* name) {
    printf("Obj_Ext1(obj:0x%p, a:%d, name:%s\n", obj, a, name);
    return 0;
}

static void Obj_Ext2(ObjectBase* obj, int a, const char* name) {
    printf("Obj_Ext2(obj:0x%p, a:%d, name:%s\n", obj, a, name);
}

static int Obj_Ext3(ObjectBase* obj, xlua::xLuaState* l) {
    int a = 0;
    const char* name = nullptr;
    l->LoadMul(1, std::tie(a, name));
    printf("Obj_Ext3(obj:0x%p, a:%d, name:%s\n", obj, a, name);
    return 0;
}

static int Obj_Ext4(ObjectBase* obj, lua_State* l) {
    int a = 0;
    const char* name = nullptr;
    a = (int)lua_tonumber(l, 1);
    name = lua_tostring(l, 2);
    printf("Obj_Ext4(obj:0x%p, a:%d, name:%s\n", obj, a, name);
    return 0;
}

static const char* Obj_GetTag1(ObjectBase* obj) {
    printf("Obj_GetTag1(obj:0x%p)\n", obj);
    return "Obj_GetTag1";
}

static int Obj_GetTag2(ObjectBase* obj, xlua::xLuaState* l) {
    printf("Obj_GetTag2(obj:0x%p)\n", obj);
    l->Push("Obj_GetTag2");
    return 1;
}

static int Obj_GetTag3(ObjectBase* obj, lua_State* l) {
    printf("Obj_GetTag3(obj:0x%p)\n", obj);
    lua_pushstring(l, "Obj_GetTag3");
    return 1;
}

static void Obj_SetTag1(ObjectBase* obj, int v) {
    printf("Obj_SetTag1(obj:0x%p), v:%d\n", obj, v);
}

static int Obj_SetTag2(ObjectBase* obj, xlua::xLuaState* l) {
    int v = l->Load<int>(1);
    printf("Obj_SetTag2(obj:0x%p), v:%d\n", obj, v);
    return 0;
}

static int Obj_SetTag3(ObjectBase* obj, lua_State* l) {
    int v = (int)lua_tonumber(l, 1);
    printf("Obj_SetTag3(obj:0x%p), v:%d\n", obj, v);
    return 0;
}

XLUA_EXPORT_EXTERNAL_BEGIN(ObjectBase)
XLUA_MEMBER_VAR_AS(obj_id, obj_id_)
XLUA_MEMBER_VAR_AS(name, name_)
XLUA_MEMBER_FUNC(AreaSize)
XLUA_MEMBER_FUNC_EXTEND(Ext1, Obj_Ext1)
XLUA_MEMBER_FUNC_EXTEND(Ext2, Obj_Ext2)
XLUA_MEMBER_FUNC_EXTEND(Ext3, Obj_Ext3)
XLUA_MEMBER_FUNC_EXTEND(Ext4, Obj_Ext4)
XLUA_MEMBER_VAR_EXTEND(Tag1, Obj_GetTag1, Obj_SetTag1)
XLUA_MEMBER_VAR_EXTEND(Tag2, Obj_GetTag2, Obj_SetTag2)
XLUA_MEMBER_VAR_EXTEND(Tag3, Obj_GetTag3, Obj_SetTag3)
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
XLUA_MEMBER_FUNC(TestParam_1)
XLUA_MEMBER_FUNC(TestParam_2)
XLUA_MEMBER_FUNC(TestParam_3)
XLUA_MEMBER_FUNC(TestParam_4)
XLUA_MEMBER_FUNC_AS(AddTriangle_1, AddTriangle, Triangle*)
XLUA_MEMBER_FUNC_AS(AddTriangle_2, AddTriangle, Triangle&)
XLUA_MEMBER_FUNC_AS(AddTriangle_3, AddTriangle, const Triangle&)
XLUA_MEMBER_FUNC_AS(AddTriangle_4, AddTriangle, const std::shared_ptr<Triangle>&)
XLUA_MEMBER_VAR_AS(type, type_)
XLUA_MEMBER_VAR_AS(width, width_)
XLUA_MEMBER_VAR_AS(height, height_)
XLUA_EXPORT_CLASS_END()

XLUA_EXPORT_CLASS_BEGIN(WeakObj)
XLUA_MEMBER_VAR_AS(index, index_)
XLUA_MEMBER_FUNC(SetArea)
XLUA_MEMBER_FUNC(SetArea2)
XLUA_EXPORT_CLASS_END();

XLUA_EXPORT_EXTERNAL_BEGIN(TestExportParams)
XLUA_MEMBER_FUNC_AS(TestBool, Test, bool)
XLUA_MEMBER_FUNC_AS(TestChar, Test, char)
XLUA_MEMBER_FUNC_AS(TestUChar, Test, unsigned char)
XLUA_MEMBER_FUNC_AS(TestShort, Test, short)
XLUA_MEMBER_FUNC_AS(TestUShort, Test, unsigned short)
XLUA_MEMBER_FUNC_AS(TestInt, Test, int)
XLUA_MEMBER_FUNC_AS(TestUInt, Test, unsigned int)
XLUA_MEMBER_FUNC_AS(TestLongLong, Test, long long)
XLUA_MEMBER_FUNC_AS(TestULongLong, Test, unsigned long long)
XLUA_MEMBER_FUNC_AS(TestFloat, Test, float)
XLUA_MEMBER_FUNC_AS(TestDouble, Test, double)
//XLUA_MEMBER_FUNC_AS(TestCharPtr, Test, char*) // compile error
XLUA_MEMBER_FUNC_AS(TestConstCharPtr, Test, const char*)
XLUA_MEMBER_FUNC_AS(TestVoidPtr, Test, void*)
XLUA_MEMBER_FUNC_AS(TestConstVoidPtr, Test, const void*)
XLUA_MEMBER_FUNC_AS(TestTrianglePtr, Test, Triangle*)
XLUA_MEMBER_FUNC_AS(TestConstTrianglePtr, Test, const Triangle*)
XLUA_MEMBER_FUNC_AS(TestTriangleRef, Test, Triangle&)
XLUA_MEMBER_FUNC_AS(TestConstTriangleRef, Test, const Triangle&)
XLUA_MEMBER_FUNC_AS(TestTriangleValue, TestValue)
//XLUA_MEMBER_FUNC_AS(TestNoneExport1, TestNoneExport, NoneExport)                  // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport2, TestNoneExport, NoneExport*)                 // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport3, TestNoneExport, std::shared_ptr<NoneExport>) // compile error
//XLUA_MEMBER_FUNC_AS(TestNoneExport4, TestNoneExport, xLuaWeakObjPtr<NoneExport>)  // compile error
XLUA_EXPORT_EXTERNAL_END()

XLUA_EXPORT_EXTERNAL_BEGIN(Global::TestStaticParams)
XLUA_MEMBER_FUNC_AS(TestBool, Test, bool)
XLUA_MEMBER_FUNC_AS(TestChar, Test, char)
XLUA_MEMBER_FUNC_AS(TestUChar, Test, unsigned char)
XLUA_MEMBER_FUNC_AS(TestShort, Test, short)
XLUA_MEMBER_FUNC_AS(TestUShort, Test, unsigned short)
XLUA_MEMBER_FUNC_AS(TestInt, Test, int)
XLUA_MEMBER_FUNC_AS(TestUInt, Test, unsigned int)
XLUA_MEMBER_FUNC_AS(TestLongLong, Test, long long)
XLUA_MEMBER_FUNC_AS(TestULongLong, Test, unsigned long long)
XLUA_MEMBER_FUNC_AS(TestFloat, Test, float)
XLUA_MEMBER_FUNC_AS(TestDouble, Test, double)
//XLUA_MEMBER_FUNC_AS(TestCharPtr, Test, char*) // compile error
XLUA_MEMBER_FUNC_AS(TestConstCharPtr, Test, const char*)
XLUA_MEMBER_FUNC_AS(TestVoidPtr, Test, void*)
XLUA_MEMBER_FUNC_AS(TestConstVoidPtr, Test, const void*)
XLUA_MEMBER_FUNC_AS(TestTrianglePtr, Test, Triangle*)
XLUA_MEMBER_FUNC_AS(TestConstTrianglePtr, Test, const Triangle*)
XLUA_MEMBER_FUNC_AS(TestTriangleRef, Test, Triangle&)
XLUA_MEMBER_FUNC_AS(TestConstTriangleRef, Test, const Triangle&)
XLUA_MEMBER_FUNC_AS(TestTriangleValue, TestValue)
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
XLUA_GLOBAL_FUNC(TestGlobal);
//XLUA_GLOBAL_VAR(global_name); // can not add var to global table
XLUA_EXPORT_GLOBAL_END();

XLUA_EXPORT_GLOBAL_BEGIN(Global)
XLUA_GLOBAL_FUNC(TestGlobal);
XLUA_GLOBAL_VAR(global_name);
XLUA_EXPORT_GLOBAL_END();


XLUA_EXPORT_CONST_BEGIN(_G)
XLUA_CONST_VAR(Version, 1)
XLUA_CONST_VAR(Name, "xlua")
XLUA_EXPORT_CONST_END()

XLUA_EXPORT_CONST_BEGIN(Const)
XLUA_CONST_VAR(Version, 1)
XLUA_CONST_VAR(Name, "xlua")
XLUA_EXPORT_CONST_END()

