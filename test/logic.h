#pragma once
#include <xlua_def.h>
#include <memory>

enum class ObjType {
    kUnknown,
    kTriangle,
    kQuard,
};

class ObjectBase {
public:
    virtual ~ObjectBase() {}

    virtual int AreaSize() const { return 0; }

    int obj_id_ = 0;
    char name_[32] ={0};
    int Tag1;
    int Tag2;
    int Tag3;
};

class Triangle : public ObjectBase {
public:
    XLUA_DECLARE_CLASS(Triangle, ObjectBase);
public:
    int AreaSize() const override { return line_1_ * line_2_ * line_3_; }

    int line_1_ = 0;
    int line_2_ = 0;
    int line_3_ = 0;
};

class IName {
public:
    virtual ~IName() { }
    virtual const char* Name() const = 0;
};

class ExtType {
public:
    void DoExtWork(int) { printf("do extern work\n"); }

    int type_ = 0;
};

class Quard : public IName, public Triangle, public ExtType {
public:
    const char* Name() const override { return "Quard"; }
    XLUA_DECLARE_CLASS(Quard, Triangle);

public:
    int AreaSize() const override { return width_ * height_; }

    void AddTriangle(Triangle*) {
        printf("void AddTriangle(Triangle*)\n");
    }

    void AddTriangle(Triangle&) {
        printf("void AddTriangle(Triangle)\n");
    }

    void AddTriangle(const Triangle&) {
        printf("void AddTriangle(const Triangle&)\n");
    }

    void AddTriangle(const std::shared_ptr<Triangle>&) {
        printf("void AddTriangle(std::shared_ptr<Triangle>)\n");
    }

    int TestParam_1(int a, const char* name, Triangle* triangle) {
        printf("TestParam_1(this:0x%p, a:%d, name:%s, triangle:0x%p)\n", this, a, name, triangle);
        return 1;
    }

    void TestParam_2(int a, const char* name, Triangle* triangle) {
        printf("TestParam_2(this:0x%p, a:%d, name:%s, triangle:0x%p)\n", this, a, name, triangle);
    }

    int TestParam_3(int a, const char* name, Triangle* triangle) const {
        printf("TestParam_3(this:0x%p, a:%d, name:%s, triangle:0x%p)\n", this, a, name, triangle);
        return 1;
    }

    void TestParam_4(int a, const char* name, Triangle* triangle) const {
        printf("TestParam_4(this:0x%p, a:%d, name:%s, triangle:0x%p)\n", this, a, name, triangle);
    }

    int width_ = 0;
    int height_= 0;
};

struct Color {
    int a = 0;
    int r = 0;
    int g = 0;
    int b = 0;
};

struct Vec2 {
    float x = 0;
    float y = 0;
};

struct PushVal {
    int a = 0;
    int b = 0;
};

struct WeakObj : public WeakObjBase {
    int index_ = 0;

    void SetArea(ObjectBase* obj) {
        printf("void SetArea(ObjectBase* obj)");
    }

    void SetArea2(ObjectBase& obj) {
        printf("void SetArea(ObjectBase& obj)");
    }

public:
    XLUA_DECLARE_CLASS(WeakObj);
};

#define _TEST_PARAM(Type)  Type Test(Type val) { printf("Test("#Type" val)\n"); return val; }

struct NoneExport {};

struct TestExportParams {
    _TEST_PARAM(bool)
    _TEST_PARAM(char)
    _TEST_PARAM(unsigned char)
    _TEST_PARAM(short)
    _TEST_PARAM(unsigned short)
    _TEST_PARAM(int)
    _TEST_PARAM(unsigned int)
    _TEST_PARAM(long long)
    _TEST_PARAM(unsigned long long)
    _TEST_PARAM(float)
    _TEST_PARAM(double)
    _TEST_PARAM(char*)  // error
    _TEST_PARAM(const char*)
    _TEST_PARAM(void*)
    _TEST_PARAM(const void*)

    void Test(Triangle*) {}
    void Test(const Triangle*) {}
    void Test(Triangle&) {}
    void Test(const Triangle&) {}
    void TestValue(Triangle) {}

    void TestNoneExport(NoneExport) {}  // error
    void TestNoneExport(NoneExport*) {} // error
    void TestNoneExport(std::shared_ptr<NoneExport>) {} // error
    void TestNoneExport(xLuaWeakObjPtr<NoneExport>) {}  // error
};

namespace Global {
#define _TEST_STATIC_PARAM(Type) static Type Test(Type val) { printf("Test("#Type" val)\n"); return val; }
    struct TestStaticParams {
        _TEST_STATIC_PARAM(bool)
        _TEST_STATIC_PARAM(char)
        _TEST_STATIC_PARAM(unsigned char)
        _TEST_STATIC_PARAM(short)
        _TEST_STATIC_PARAM(unsigned short)
        _TEST_STATIC_PARAM(int)
        _TEST_STATIC_PARAM(unsigned int)
        _TEST_STATIC_PARAM(long long)
        _TEST_STATIC_PARAM(unsigned long long)
        _TEST_STATIC_PARAM(float)
        _TEST_STATIC_PARAM(double)
        _TEST_STATIC_PARAM(char*)  // error
        _TEST_STATIC_PARAM(const char*)
        _TEST_STATIC_PARAM(void*)
        _TEST_STATIC_PARAM(const void*)

        static void Test(Triangle*) {}
        static void Test(const Triangle*) {}
        static void Test(Triangle&) {}
        static void Test(const Triangle&) {}
        static void TestValue(Triangle) {}

        static void TestNoneExport(NoneExport) {}  // error
        static void TestNoneExport(NoneExport*) {} // error
        static void TestNoneExport(std::shared_ptr<NoneExport>) {} // error
        static void TestNoneExport(xLuaWeakObjPtr<NoneExport>) {}  // error
    };
}
