#include "test.h"
#include "lua_export.h"
#include <xlua_state.h>
#include <string.h>

static int sLuaCall_1(xlua::State* l) {
    printf("int sLuaCall_1(xlua::State* l)\n");
    auto f = l->GetField<lua_CFunction>(1, "Call_1");
    // 非标准Lua导出函数会通过中转函数转发
    assert((void*)f != &sLuaCall_1);
    return 0;
}

static int sLuaCall_2(lua_State* l) {
    printf("int sLuaCall_2(lua_State* l)\n");

    lua_getfield(l, 1, "Call_2");
    auto f = lua_tocfunction(l, -1);
    assert((void*)f == &sLuaCall_2);
    return 0;
}

static int sLuaCall_3(xlua::State* l) {
    printf("int sLuaCall_3(xlua::State* l)\n");
    return 0;
}

#define _TEST_PUSH_LOAD_BASIC(Var)  \
    l->Push(Var);   \
    assert(Var == l->Get<decltype(Var)>(-1))

static void sPushLoadBasic(xlua::State* l) {
    xlua::StackGuard guard(l->GetLuaState());
    bool b = true;
    char c = 'a';
    unsigned char uc = 'b';
    short s = 101;
    unsigned short us = 102;
    int i = 1001;
    unsigned int ui = 1002;
    long ln = 10001;
    unsigned long ul = 10002;
    long long ll = 10000001;
    unsigned long long ull = 10000002;
    float f = 1.11f;
    double d = 1.111f;
    const char* p = "xlua";

    _TEST_PUSH_LOAD_BASIC(b);
    _TEST_PUSH_LOAD_BASIC(c);
    _TEST_PUSH_LOAD_BASIC(uc);
    _TEST_PUSH_LOAD_BASIC(s);
    _TEST_PUSH_LOAD_BASIC(us);
    _TEST_PUSH_LOAD_BASIC(i);
    _TEST_PUSH_LOAD_BASIC(ui);
    _TEST_PUSH_LOAD_BASIC(ln);
    _TEST_PUSH_LOAD_BASIC(ul);
    _TEST_PUSH_LOAD_BASIC(ll);
    _TEST_PUSH_LOAD_BASIC(ull);
    _TEST_PUSH_LOAD_BASIC(f);
    _TEST_PUSH_LOAD_BASIC(d);

    l->Push(p);
    assert(strcmp(p, l->Get<const char*>(-1)) == 0);

    l->Push((const void*)p);
    assert(l->Get<const void*>(-1) == p);

    assert(l->GetTop() == 15);

    {
        bool l_b = false;
        char l_c = 0;
        unsigned char l_uc = 0;
        short l_s = 0;
        unsigned short l_us = 0;
        int l_i = 0;
        unsigned int l_ui = 0;
        long l_ln = 0;
        unsigned long l_ul = 0;
        long long l_ll = 0;
        unsigned long long l_ull = 0;
        float l_f = 0.0f;
        double l_d = 0.0;
        const char* l_p = nullptr;
        void* l_v = nullptr;

        l->GetMul(1, std::tie(l_b, l_c, l_uc, l_s, l_us, l_i, l_ui,
            l_ln, l_ul, l_ll, l_ull, l_f, l_d, l_p, l_v));
        assert(b == l_b);
        assert(c == l_c);
        assert(uc == l_uc);
        assert(s == l_s);
        assert(us == l_us);
        assert(i == l_i);
        assert(ui == l_ui);
        assert(ln == l_ln);
        assert(ul == l_ul);
        assert(ll == l_ll);
        assert(ull == l_ull);
        assert(f == l_f);
        assert(d == l_d);
        assert(strcmp(p, l_p) == 0);
        assert(l_v == p);
    }
}

static void sPushLoadExtend(xlua::State* l) {
    //xlua::StackGuard guard(l->GetLuaState());
    //Color c{1, 2, 3, 4};

    //l->Push(c);
    ////l->Push(&c);  // can not push extend type pointer

    //auto d1 = l->Get<Color>(-1);
    //assert(c.a == d1.a && c.r == d1.r && c.g == d1.g && c.b == d1.b);

    //auto d2 = l->Get<const Color>(-1);
    //assert(c.a == d2.a && c.r == d2.r && c.g == d2.g && c.b == d2.b);

    ////auto d3 = l->Get<const Color&>(-1);  // can not load reference value

    //PushVal pv;
    //l->Push(pv);

    //l->Get<PushVal>(-1); // has not declared load from lua
}

static void sPushLoadDeclare(xlua::State* l) {
    xlua::StackGuard guard(l->GetLuaState());
    Square quard;
    const ShapeBase* const cbc = &quard;
    const ShapeBase* cb = &quard;
    ShapeBase*const bc = &quard;

    l->Push(bc);
    //l->Push(cb);  // can not push const value pointer
    //l->Push(cbc); // can not push const value pointer

    const ShapeBase* const cbc2 = l->Get<decltype(cbc)>(-1);
    ShapeBase* const bc2 = l->Get<decltype(bc)>(-1);
    cb = l->Get<decltype(cb)>(-1);

    assert(cbc == cbc2 && bc2 == bc);

    // allow pointer to value, if pointer is nullptr, return default value
    //auto base_value1 = l->Get<ShapeBase>(-1);
    //auto base_value2 = l->Get<const ShapeBase>(-1);
    //auto base_value3 = l->Get<ShapeBase>(-2);         // log error, obj is nil
    //auto base_value4 = l->Get<const ShapeBase>(-2);   // log error, obj is nil

    Triangle* triangle = l->Get<Triangle*>(-1);        // log error, type error
    assert(triangle == nullptr);    // type not allow

    l->Push(&quard);

    /* 子类能够转换为基类 */
    auto triangle_ptr_2 = l->Get<Triangle*>(-1);
    auto base_ptr_2 = l->Get<ShapeBase*>(-1);

    /* 这里开启多继承才能正确转换 */
#if XLUA_ENABLE_MULTIPLE_INHERITANCE
    assert(triangle_ptr_2 == &quard);
    assert(base_ptr_2 == &quard);
#else
    assert(triangle_ptr_2 != &quard);
    assert(base_ptr_2 != &quard);
#endif

    l->Push(quard);
    /* value -> ptr */
//    auto triangle_ptr_3 = l->Get<Triangle*>(-1);
//    auto base_ptr_3 = l->Get<ShapeBase*>(-1);
//
//    /* to super type value */
//    auto triangle_3 = l->Get<Triangle>(-1);
//    auto base_3 = l->Get<ShapeBase>(-1);
//
//#if XLUA_ENABLE_MULTIPLE_INHERITANCE
//    assert(base_ptr_3 == triangle_ptr_3);
//#else
//    assert(base_ptr_3 != triangle_ptr_3);
//#endif

    auto s_q = std::make_shared<Square>();
    l->Push(s_q);

    auto triangle_ptr_4 = l->Get<Triangle*>(-1);
    auto base_ptr_4 = l->Get<ShapeBase*>(-1);

    /* ptr->value, to super type value */
    auto triangle_4 = l->Get<Triangle*>(-1);
    auto base_4 = l->Get<ShapeBase*>(-1);
    auto quard_4 = l->Get<Square*>(-1);

    auto s_q_2 = l->Get<std::shared_ptr<Square>>(-1);
    auto s_q_3 = l->Get<std::shared_ptr<const Square>>(-1);
    assert(s_q == s_q_2);
    assert(s_q_3 == s_q_2);

    /* to super type shared_ptr */
    auto s_t_1 = l->Get<std::shared_ptr<Triangle>>(-1);
    auto s_b_1 = l->Get<std::shared_ptr<ShapeBase>>(-1);

#if XLUA_ENABLE_MULTIPLE_INHERITANCE
    assert(base_ptr_4 == triangle_ptr_4);
    assert(s_b_1 == s_t_1);
#else
    assert(base_ptr_4 != triangle_ptr_4);
    assert(s_b_1 != s_t_1);
#endif
}

namespace {
    struct TmpObj {
        TmpObj() {
            printf("std::function construct~\n");
        }

        ~TmpObj() {
            printf("std::function destruct~\n");
        }

        TmpObj(const TmpObj&) = delete;
        TmpObj& operator = (const TmpObj&) = delete;
    };
}

void sPushLoadFunction(xlua::State* l) {
    xlua::StackGuard guard(l->GetLuaState());
    int(*f_1)() = []()-> int {
        printf("[]()-> int\n");
        return 1;
    };

    const char*(*f_2)(int) = [](int a) -> const char* {
        printf("[](int a:%d) -> const char*\n", a);
        return "xlua";
    };

    bool(*f_3)(const std::shared_ptr<Square>&)  = [](const std::shared_ptr<Square>& qa) -> bool {
        printf("[](const std::shared_ptr<Square>& qa=0x%p) -> bool\n", qa.get());
        return true;
    };

    void(*f_4)(Square& quard) = [](Square& quard) -> void {
        quard.obj_id_ = 1;
        printf("[](Square& squard) -> void\n");
    };

    int(*f_5)(lua_State*) = [](lua_State* l) -> int {
        printf("[](lua_State* l) -> int\n");
        lua_pushboolean(l, 1);
        lua_pushfstring(l, "xlua");
        lua_pushnumber(l, 1.01);
        return 3;
    };

    int(*f_6)(xlua::State*) = [](xlua::State* l) -> int {
        printf("[](xlua::State* l) -> int\n");
        l->PushMul(true, "xlua", 1.01f);
        return 3;
    };

    {
        Square quard;
        auto s_q = std::make_shared<Square>();
        int int_val = 0;
        bool bool_val = false;
        float float_val = 0.0f;
        const char* str_val = nullptr;
        l->Push(f_1);
        if (auto guard = l->Call(std::tie(int_val))) {
            assert(int_val == 1);
        }

        l->Push(f_2);
        if (auto guard = l->Call(std::tie(str_val), 2)) {
            assert(strcmp(str_val, "xlua") == 0);
        }

        l->Push(f_3);
        if (l->Call(std::tie(bool_val), nullptr)) {
            assert(true);   // nullptr -> shared_ptr
        }

        l->Push(f_3);
        if (l->Call(std::tie(bool_val), &quard)) {
            assert(false);  // param not allow
        }

        l->Push(f_3);
        if (l->Call(std::tie(bool_val), s_q)) {
            assert(bool_val);  // param not allow
        }

        l->Push(f_4);
        if (l->Call(std::tie(), quard)) {
            assert(quard.obj_id_ == 0); // value can not recive data
        }

        l->Push(f_4);
        if (l->Call(std::tie(), &quard)) {
            assert(quard.obj_id_ == 1);
        }

        l->Push(f_4);
        if (l->Call(std::tie(), s_q)) {
            assert(s_q->obj_id_ == 1);
        }

        l->Push(f_5);
        if (l->Call(std::tie(bool_val, str_val, float_val))) {
            assert(bool_val);
            assert(strcmp(str_val, "xlua") == 0);
            assert(float_val == 1.01f);
        }

        l->Push(f_6);
        if (l->Call(std::tie(bool_val, str_val, float_val))) {
            assert(bool_val);
            assert(strcmp(str_val, "xlua") == 0);
            assert(float_val == 1.01f);
        }

        l->Push(s_q);
        l->LoadField(-1, "TestParam_1");
        if (l->Call(std::tie(int_val), &quard, 101, "xlua", &quard)) {
            assert(int_val == 1);
        }

        l->LoadField(-1, "TestParam_2");
        if (l->Call(std::tie(), &quard, 101, "xlua", &quard)) {
            assert(true);
        }

        l->LoadField(-1, "TestParam_3");
        if (l->Call(std::tie(int_val), &quard, 101, "xlua", &quard)) {
            assert(int_val == 1);
        }

        l->LoadField(-1, "TestParam_4");
        if (l->Call(std::tie(), &quard, 101, "xlua", &quard)) {
            assert(true);
        }

        l->PopTop(1);   // quard
        assert(l->GetTop() == 0);
    }

    {
        auto s_o = std::make_shared<TmpObj>();
        std::function<void(int, const char*, void*)> f_5 = [=](int s, const char* name, void* p) {
            printf("void(int:%d, const char*:%s, void*:0x%p)\n", s, name, p);
            assert(p == s_o.get());
        };
        void* p_v = s_o.get();
        s_o = nullptr;

        l->Push(f_5);
        if (l->Call(std::tie(), 1, "xlua", p_v)) {
            assert(true);
        }

        std::function<void(int, int, int)> f_6 = [=](int a, int b, int c) {
            printf("void(a:%d, b:%d, c:%d)\n", a, b, c);
        };
        l->Push(f_6);
        if (l->Call(std::tie(), 1, 2, 3)) {
            assert(true);
        }
    }
}

static void sPushLoadTable(xlua::State* l) {
    bool is_ok = false;
    l->NewTable();
    // set value and pop the value
    is_ok = l->SetGlobal("Test.Table.Instance", false);
    assert(is_ok == false);
    assert(l->GetTop() == 0);

    l->Push(1);
    is_ok = l->SetGlobal(nullptr, false);
    assert(is_ok == false);
    assert(l->GetTop() == 0);

    // create global table
    l->NewTable();
    is_ok = l->SetGlobal("Test.Table.Instance", true);
    assert(is_ok);
    assert(l->GetTop() == 0);

    // remove global tables
    //is_ok = l->SetGlobal("Test.Table.Instance", nullptr, false);
    //assert(is_ok);
    //is_ok = l->SetGlobal("Test.Table", nullptr, false);
    //assert(is_ok);
    //is_ok = l->SetGlobal("Test", nullptr, false);
    //assert(is_ok);
    //assert(l->GetTop() == 0);

    xlua::VarType ty = xlua::VarType::kNil;
    // load value on the stack, is failed push nil on the stack
    ty = l->LoadGlobal("");
    assert(ty == xlua::VarType::kNil);
    assert(l->GetType(-1) == xlua::VarType::kNil);
    assert(l->GetTop() == 1);
    l->PopTop(1);

    ty = l->LoadGlobal("Test");
    assert(ty == xlua::VarType::kNil);
    assert(l->GetType(-1) == xlua::VarType::kNil);
    assert(l->GetTop() == 1);
    l->PopTop(1);

    ty = l->LoadGlobal("Test.Table");
    assert(ty == xlua::VarType::kNil);
    assert(l->GetType(-1) == xlua::VarType::kNil);
    assert(l->GetTop() == 1);
    l->PopTop(1);

    // set global var failed
    //is_ok = l->SetGlobal("Test.Table.Instance", 1, false);
    assert(is_ok == false);
    assert(l->GetTop() == 0);

    // create table & set global table
    //l->SetGlobal("Test.Table.Instance", l->CreateTable(), true);
    //xlua::Table table = l->GetGlobal<xlua::Table>("Test.Table.Instance");
    //assert(table);
    assert(l->GetTop() == 0);

    //is_ok = l->SetField(table, "Call_1", &sLuaCall_1);
    //assert(is_ok);
    //assert(l->GetTop() == 0);

    //is_ok = l->SetField(table, "Call_2", &sLuaCall_2);
    //assert(is_ok);
    //assert(l->GetTop() == 0);

    //is_ok = l->SetField(table, "Call_3", &sLuaCall_3);
    //assert(is_ok);
    //assert(l->GetTop() == 0);

    //if (!l->Call(table, "Call_1", std::tie()))
    //    assert(false);
    //assert(l->GetTop() == 0);

    //if (!l->Call(table, "Call_2", std::tie()))
    //    assert(false);
    //assert(l->GetTop() == 0);

    //if (!l->Call(table, "Call_3", std::tie()))
    //    assert(false);
    //assert(l->GetTop() == 0);

    //xlua::Function f = l->GetField<xlua::Function>(table, "Call_2");
    //assert(f);
    //assert(l->GetTop() == 0);

    //if (!l->Call(f, std::tie(), table))
    //    assert(false);
}

void TestPushLoad(xlua::State* l)
{
    int top = l->GetTop();

    sPushLoadBasic(l);
    sPushLoadExtend(l);
    sPushLoadDeclare(l);
    sPushLoadFunction(l);
    sPushLoadTable(l);

    assert(top == l->GetTop());
}

