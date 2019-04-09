#pragma once
#include <xlua_def.h>
#include <memory>

class ObjectBase {
public:
    virtual ~ObjectBase() {}

    virtual int AreaSize() const { return 0; }

    int obj_id_ = 0;
    char name_[32] ={0};
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

class Quard : public IName, public ObjectBase, public ExtType {
public:
    const char* Name() const override { return "MultyInherit"; }
    XLUA_DECLARE_CLASS(Quard, ObjectBase);

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
