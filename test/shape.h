#pragma once
#include <xlua_def.h>
#include <memory>

enum class ShapeType {
    kUnknown,
    kTriangle,
    kQuard,
};

class ShapeBase {
public:
    virtual ~ShapeBase() {}

    virtual int AreaSize() const { return 0; }

    int obj_id_ = 0;
    char name_[32] ={0};
    int Tag1;
    int Tag2;
    int Tag3;
};

class Triangle : public ShapeBase {
public:
    XLUA_DECLARE_CLASS(Triangle, ShapeBase);
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

class Square : public IName, public Triangle, public ExtType {
public:
    const char* Name() const override { return "Square"; }
    XLUA_DECLARE_CLASS(Square, Triangle);

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
    Color() {}
    Color(int _a, int _r, int _g, int _b) : a(_a), r(_r), g(_g), b(_b)
    {}

    int a = 0;
    int r = 0;
    int g = 0;
    int b = 0;
};

struct Vec2 {
    Vec2() { }
    Vec2(float _x, float _y) : x(_x), y(_y) { }
    float x = 0;
    float y = 0;
};

struct PushVal {
    PushVal() {}
    PushVal(int _a, int _b) : a(_a), b(_b)
    {}

    int a = 0;
    int b = 0;
};

struct WeakObj : public WeakObjBase {
    int index_ = 0;

    void SetArea(ShapeBase* obj) {
        printf("void SetArea(ShapeBase* obj)");
    }

    void SetArea2(ShapeBase& obj) {
        printf("void SetArea(ShapeBase& obj)");
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
