#pragma once
#include <xlua_def.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

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
    XLUA_DECLARE_OBJ_INDEX;
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
    //void TestNoneExport(xLuaWeakObjPtr<NoneExport>) {}  // error
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
        //static void TestNoneExport(xLuaWeakObjPtr<NoneExport>) {}  // error
    };
}

/* life time */
struct LifeTime {
    LifeTime() { ++ s_counter; }
    LifeTime(const LifeTime&) { ++s_counter; }
    ~LifeTime() { --s_counter; }

    static int s_counter;
};

/* member and static member */
struct ExportObj {
};

struct NoneExportObj {
};

struct TestMember {
    bool boolean_val;
    int int_val;
    long long_val;
    char name_val[64]{0};
    ExportObj export_val;
    NoneExportObj none_export_va;
    std::vector<int> vector_val;
    std::map<std::string, TestMember*> map_val;
    int m_lua_name__;
    int read_only;

    void Test(const char* name) {
        printf("TestMember.Test called %s\n", name);
    }

    static bool s_boolean_val;
    static int s_int_val;
    static long s_long_val;
    static char s_name_val[64];
    static ExportObj s_export_val;
    static NoneExportObj s_none_export_va;
    static std::vector<int> s_vector_val;
    static std::map<std::string, TestMember*> s_map_val;
    static int s_lua_name__;
    static int s_read_only;

    static void sTest(const char* name) {
        printf("TestMember.sTest called %s\n", name);
    }
};

/* overlaod functions */
struct OverloadMember {
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
    _TEST_PARAM(ExportObj*)
    _TEST_PARAM(const ExportObj*)
    _TEST_PARAM(ExportObj&)
    _TEST_PARAM(const ExportObj&)
    _TEST_PARAM(std::shared_ptr<ExportObj>)
    void TestValue(ExportObj) {}
    _TEST_PARAM(NoneExportObj)
    _TEST_PARAM(NoneExportObj*)
    _TEST_PARAM(std::shared_ptr<NoneExportObj>)
};

namespace OverloadGlobal {
    inline _TEST_PARAM(bool)
    inline _TEST_PARAM(char)
    inline _TEST_PARAM(unsigned char)
    inline _TEST_PARAM(short)
    inline _TEST_PARAM(unsigned short)
    inline _TEST_PARAM(int)
    inline _TEST_PARAM(unsigned int)
    inline _TEST_PARAM(long long)
    inline _TEST_PARAM(unsigned long long)
    inline _TEST_PARAM(float)
    inline _TEST_PARAM(double)
    inline _TEST_PARAM(char*)  // error
    inline _TEST_PARAM(const char*)
    inline _TEST_PARAM(void*)
    inline _TEST_PARAM(const void*)
    inline _TEST_PARAM(ExportObj*)
    inline _TEST_PARAM(const ExportObj*)
    inline _TEST_PARAM(ExportObj&)
    inline _TEST_PARAM(const ExportObj&)
    inline _TEST_PARAM(std::shared_ptr<ExportObj>)
    inline void TestValue(ExportObj) {}
    inline _TEST_PARAM(NoneExportObj)
    inline _TEST_PARAM(NoneExportObj*)
    inline _TEST_PARAM(std::shared_ptr<NoneExportObj>)
};

/* simple object inherit */
struct IRenderer {
};

enum class WidgetType {
    kImage,
    kLabel,
    kButton,
};

struct Widget {
protected:
    Widget(WidgetType type) : type_(type), parent(nullptr) {}

public:
    WidgetType GetType() const { return type_; }

public:
    virtual void Render(IRenderer*) = 0;

private:
    WidgetType type_;

public:
    Widget* parent;
    Color color;
    Vec2 size;
    Vec2 pos;
};

struct Image : Widget {
    Image() : Widget(WidgetType::kImage) {}

    void Render(IRenderer* renderer) {
        printf("render image {texture:%s}\n", texture.c_str());
    }

    std::string texture;
};

struct Label : Widget {
    Label() : Widget(WidgetType::kLabel) {}

    void Render(IRenderer* renderer) {
        printf("render label {text:%s, font:%s, font_size:%jd}\n", text.c_str(), font.c_str(), font_size);
    }

    std::string text;
    std::string font;
    size_t font_size;
};

struct Button : Widget {
    Button() : Widget(WidgetType::kButton) {
        label.parent = this;
    }

    void Render(IRenderer* renderer) {
        printf("render button\n");
        label.Render(renderer);
    }

    Label label; // index as ptr?
    std::function<void(Button*)> onclick;
};

/* xlua weak object */
struct Object {
    XLUA_DECLARE_OBJ_INDEX;

    ~Object() {
        strcpy_s(name, 64, "destructed");
    }

    virtual int Update(int delta) = 0;

    int id_;
    char name[64]{0};
};

struct Character : Object {
    int Update(int delta) override {
        printf("Character.Update(%d)\n", delta);
        return delta;
    }

    int hp;
};

struct Doodad : Object {
    int Update(int delta) override {
        printf("Doodad.Update(%d)\n", delta);
        return delta;
    }

    int drop_id;
};

/* other weak object ptr */
struct WeakObj {
    ~WeakObj();

private:
    friend class WeakObjArray;

    int index_ = 0;
};

struct WeakObjPtrBase {
    bool IsValid() const;
    WeakObj* GetPtr() const;
    void Set(WeakObj* ptr);

    inline void Set(const WeakObjPtrBase& other) {
        index_ = other.index_;
        serial_ = other.serial_;
    }

    inline bool Equal(const WeakObjPtrBase& other) const {
        return index_ == other.index_ && serial_ == other.serial_;
    }

    inline void Reset() {
        index_ = 0;
        serial_ = 0;
    }

private:
    int index_ = 0;
    int serial_ = 0;
};

template <typename Ty>
struct WeakObjPtr : private WeakObjPtrBase {
    static_assert(std::is_base_of<WeakObj, Ty>::value, "only weak obj");

    WeakObjPtr() = default;
    WeakObjPtr(Ty* obj) { Set(obj); }

    template <typename U>
    WeakObjPtr(const WeakObjPtr<U>& other) {
        static_assert(std::is_convertible<U*, Ty*>::value, "not acceptable");
        Set(static_cast<const WeakObjPtrBase&>(other));
    }

    inline bool IsValid() const {
        return WeakObjPtrBase::IsValid();
    }

    inline Ty* GetPtr() const {
        return (Ty*)WeakObjPtrBase::GetPtr();
    }

    inline Ty* operator -> () const {
        return GetPtr();
    }

    inline Ty& operator * () const {
        return *GetPtr();
    }
};

class WeakObjArray {
    struct AryObj {
        WeakObj* ptr;
        int serial;
    };
private:
    WeakObjArray() {}
    ~WeakObjArray() {}

    WeakObjArray(const WeakObjArray&) = delete;
    WeakObjArray& operator = (const WeakObjArray&) = delete;

public:
    static WeakObjArray& Instance() {
        static WeakObjArray ary;
        return ary;
    }

public:
    int AllocIndex(WeakObj* ptr) {
        if (ptr->index_)
            return ptr->index_;

        if (empty_slots_.empty()) {
            size_t b = objs_.size();
            size_t n = b + 128;
            objs_.resize(n, AryObj{nullptr, 0});
            empty_slots_.reserve(n - 1);
            for (size_t i = n; i > b; --i)
                empty_slots_.push_back((int)(i-1));
        }

        int index = empty_slots_.back();
        empty_slots_.pop_back();

        int serial = ++serial_gener_;
        ptr->index_ = index;
        objs_[index] = AryObj{ptr, serial};
        return index;
    }

    void FreeIndex(WeakObj* ptr) {
        if (ptr == nullptr || ptr->index_ == 0)
            return;

        objs_[ptr->index_] = AryObj{nullptr, 0};
        empty_slots_.push_back(ptr->index_);
        ptr->index_ = 0;
    }

    inline WeakObj* IndexToObject(int index) {
        if (index >= (int)objs_.size())
            return nullptr;
        return objs_[index].ptr;
    }

    inline int GetSerialNumber(int index) {
        if (index >= (int)objs_.size())
            return 0;
        return objs_[index].serial;
    }

private:
    int serial_gener_ = 0;
    std::vector<AryObj> objs_{AryObj{nullptr, 0}};
    std::vector<int> empty_slots_;
};

inline WeakObj::~WeakObj() {
    if (index_)
        WeakObjArray::Instance().FreeIndex(this);
}

inline bool WeakObjPtrBase::IsValid() const {
    return index_ && WeakObjArray::Instance().GetSerialNumber(index_) == serial_;
}

inline WeakObj* WeakObjPtrBase::GetPtr() const {
    if (IsValid())
        return WeakObjArray::Instance().IndexToObject(index_);
    return nullptr;
}

inline void WeakObjPtrBase::Set(WeakObj* ptr) {
    if (ptr == nullptr) {
        index_ = 0;
        serial_ = 0;
    } else {
        index_= WeakObjArray::Instance().AllocIndex(ptr);
        serial_ = WeakObjArray::Instance().GetSerialNumber(index_);
    }
}

struct Ray {
    Vec2 orign;
    Vec2 dir;
};

struct Collider : WeakObj {
    virtual ~Collider() {}

    virtual bool IsEnable() const = 0;
    virtual void Eanble(bool eanble) = 0;
    virtual void OnEnterTrigger(Object* obj) = 0;
    virtual void OnLeaveTrigger(Object* obj) = 0;
    virtual bool RayCast(const Ray& ray) = 0;
};

struct BoxCollider : Collider {
    bool IsEnable() const final {
        return enable_;
    }

    void Eanble(bool enable) final {
        enable_ = enable;
    }

    void OnEnterTrigger(Object* obj) final {
        printf("enter box collider, obj:%s\n", obj->name);
    }

    void OnLeaveTrigger(Object* obj) final {
        printf("leave box collider, obj:%s\n", obj->name);
    }

    bool RayCast(const Ray& ray) final {
        return true;
    }

private:
    bool enable_;
};

struct CircleCollider : Collider {
    bool IsEnable() const final {
        return enable_;
    }

    void Eanble(bool enable) final {
        enable_ = enable;
    }

    void OnEnterTrigger(Object* obj) final {
        printf("enter circle collider, obj:%s\n", obj->name);
    }

    void OnLeaveTrigger(Object* obj) final {
        printf("leave circle collider, obj:%s\n", obj->name);
    }

    bool RayCast(const Ray& ray) final {
        return true;
    }

private:
    bool enable_;
};

struct CapsuleCollider : CircleCollider {
};

/* multi inherit test */
