#include "export_prepare.h"
#include "xlua_export.h"
#include "xlua_state.h"
#include "xlua.h"

int Derived::sIdx = 0;

const xlua::TypeDesc* xLuaGetTypeDesc(xlua::Identity<TestObj>) {
    using meta = xlua::internal::Meta<Derived>;
    using StringView = xlua::internal::StringView;
    using xlua::internal::PurifyMemberName;
    using xlua::internal::GetState;

    static const xlua::TypeDesc* desc = []()->const xlua::TypeDesc* {
        auto* factory = xlua::CreateFactory<TestObj>("TestObj");
        factory->AddMember(false, "test", [](lua_State*l)->int {
            StringView name("test");
            return meta::Call(GetState(l), desc, name, &TestObj::test);
        });

        factory->AddMember(false, "a", [](xlua::State* s, void* obj, const xlua::TypeDesc* info) {
            return meta::Get(s, obj, info, desc, &TestObj::a);
        }, [](xlua::State* s, void* obj, const xlua::TypeDesc* info) {
            constexpr StringView name("a");
            return meta::Set(s, obj, info, desc, name, &TestObj::a);
        });
        return factory->Finalize();
    }();

    return desc;
}

const xlua::TypeDesc* xLuaGetTypeDesc(xlua::Identity<Derived>) {
    using meta = xlua::internal::Meta<Derived>;
    using StringView = xlua::internal::StringView;
    using xlua::internal::PurifyMemberName;
    using xlua::internal::GetState;

    static const xlua::TypeDesc* desc = []()->const xlua::TypeDesc* {
        auto* factory = xlua::CreateFactory<Derived, TestObj>("Derived");

        factory->AddMember(false, "print", [](lua_State*l)->int {
            return meta::Call(GetState(l), desc, PurifyMemberName("print"), &Derived::print);
        });

        factory->AddMember(false, "szName", [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Get(l, obj, info, desc, &Derived::szName);
        }, [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            constexpr StringView name = PurifyMemberName("szName"); // this will avoid purify name every call
            return meta::Set(l, obj, info, desc, name, &Derived::szName);
        });

        factory->AddMember(true, "StaticCall", [](lua_State*l)->int {
            return meta::Call(GetState(l), desc, PurifyMemberName("print"), &Derived::StaticCall);
        });

        factory->AddMember(true, "sIdx", [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Get(l, obj, info, desc, &Derived::sIdx);
        }, [](xlua::State* l, void* obj, const xlua::TypeDesc* info) {
            return meta::Set(l, obj, info, desc, PurifyMemberName("sIdx"), &Derived::sIdx);
        });

        return factory->Finalize();
    }();

    return desc;
}

/* register to xlua system */
namespace {
    struct TypeNode_1 : xlua::internal::TypeNode {
        void Reg() override {
            xLuaGetTypeDesc(xlua::Identity<TestObj>());
        }
    } g1;

    struct TypeNode_2 : xlua::internal::TypeNode {
        void Reg() override {
            xLuaGetTypeDesc(xlua::Identity<Derived>());
        }
    } g2;
}

struct TestNone;

XLUA_DECLARE_CLASS(TestNone);

struct TestNone {
};

enum class TestEnum {
    kNone,
    kOne,
    kTwo,
};

static void test_load_push(xlua::State* l) {
    l->Push(1);
    l->Push(true);
    const char* p = "str";
    l->Push(p);
    l->Push("array");

    bool btype = l->IsType<int, bool, const char*, std::string>(1);
    btype = l->IsType<bool, bool, const char*, const char*>(1);
    btype = l->IsType<>(1);                 // always true
    btype = l->IsType<void>(1);             // check is nil
    btype = l->IsType<std::nullptr_t>(1);   // check is nil

    char buf[] = "normal array";
    l->Push(buf);

    int i;
    bool b;
    std::string s;
    l->LoadMul(1, std::tie(i, b, s));

    TestObj obj;
    l->Push(TestObj());
    l->Push(obj);
    l->Push(&obj);
    l->Push(TestEnum::kTwo);
    TestEnum te_1 = l->Load<TestEnum>(-1);

    auto sp = std::make_shared<TestObj>();
    l->Push(sp);
    l->Push(std::make_shared<TestObj>());
    l->Load<std::shared_ptr<TestObj>>(-1);

    std::function<int(int)> f = [](int i) {
        return i + 1;
    };
    l->Push(f);
    auto f2 = l->Load<std::function<int(int)>>(-1);

    std::function<void(int)> f3 = [](int i) {
    };
    l->Push(f3);
    auto f4 = l->Load<std::function<void(int)>>(-1);

    std::vector<int> vec_int;
    l->Push(vec_int);
    l->Load<std::vector<int>*>(-1);

    std::vector<std::vector<int>> vec_int2;
    l->Push(vec_int2);
    l->Load<std::vector<std::vector<int>>*>(-1);

    std::map<int, std::vector<const char*>> map_int;
    l->Push(map_int);
    l->Load<std::map<int, std::vector<const char*>>*>(-1);
}

static const char* kTestDeclaredObj = R"V0G0N(
local obj = ...
obj.a = 1
obj:test()
obj.szName = "object"
print("1111111", obj, obj.szName)
obj:print()
Derived.sIdx = 101
print("2222222", obj.sIdx, Derived.sIdx)
local s = Derived.StaticCall(obj)
print("3333333", s)
Derived.StaticCall(nil)
)V0G0N";

static void test_delcared_obj(xlua::State* s) {
    Derived d;
    luaL_loadbuffer(s->GetLuaState(), kTestDeclaredObj, 
        ::strlen(kTestDeclaredObj), "test_delcared_obj");
    s->Push(&d);
    if (LUA_OK != lua_pcall(s->GetLuaState(), 1, 0, 0))
        printf("call failed:%s\n", lua_tostring(s->GetLuaState(), -1));

    printf("d1:%d d2:%d s:%s \n", d.a, Derived::sIdx, d.szName);
}

void test_export() {
    xlua::State* s = xlua::CreateState(nullptr);

    test_delcared_obj(s);

    xlua::DestoryState(s);
}
