#include "lua_export.h"
#include "xlua.h"
#include "gtest/gtest.h"

#define _TEST_CHECK_VAL(Type, Val)                          \
    {                                                       \
        Type v;                                             \
        auto guard = s->Call("Check", std::tie(v), Val);    \
        ASSERT_TRUE(guard);                                 \
        ASSERT_EQ(v, Val);                                  \
    }

#define _TEST_CHECK_VAL_NOT(Type, Val)                      \
    {                                                       \
        Type v;                                             \
        auto guard = s->Call("Check", std::tie(v), Val);    \
        ASSERT_TRUE(guard);                                 \
        ASSERT_NE(v, Val);                                  \
    }

TEST(xlua, PushLoadNormal) {
    const char* check_func = "function Check(...) return ... end";
    auto* s = xlua::Create(nullptr);

    ASSERT_TRUE(s->DoString(check_func, "Check"));

    _TEST_CHECK_VAL(bool, true);
    _TEST_CHECK_VAL(bool, false);

    _TEST_CHECK_VAL(char, 'a');
    _TEST_CHECK_VAL(unsigned char, 0xff);

    _TEST_CHECK_VAL(short, 1);
    _TEST_CHECK_VAL(short, -1);
    _TEST_CHECK_VAL(short, std::numeric_limits<short>::min());
    _TEST_CHECK_VAL(short, std::numeric_limits<short>::max());
    //_TEST_CHECK_VAL(short, std::numeric_limits<short>::signaling_NaN());
    //_TEST_CHECK_VAL(short, std::numeric_limits<short>::quiet_NaN());

    _TEST_CHECK_VAL(unsigned short, 1);
    _TEST_CHECK_VAL(unsigned short, (unsigned short)-1);
    _TEST_CHECK_VAL(unsigned short, std::numeric_limits<unsigned short>::min());
    _TEST_CHECK_VAL(unsigned short, std::numeric_limits<unsigned short>::max());

    _TEST_CHECK_VAL(int, 1);
    _TEST_CHECK_VAL(int, (int)-1);
    _TEST_CHECK_VAL(int, std::numeric_limits<int>::min());
    _TEST_CHECK_VAL(int, std::numeric_limits<int>::max());
    //_TEST_CHECK_VAL(int, std::numeric_limits<int>::signaling_NaN());
    //_TEST_CHECK_VAL(int, std::numeric_limits<int>::quiet_NaN());

    _TEST_CHECK_VAL(unsigned int, 1);
    _TEST_CHECK_VAL(unsigned int, (unsigned int)-1);
    _TEST_CHECK_VAL(unsigned int, std::numeric_limits<unsigned int>::min());
    _TEST_CHECK_VAL(unsigned int, std::numeric_limits<unsigned int>::max());

    _TEST_CHECK_VAL(long, 1);
    _TEST_CHECK_VAL(long, -1);
    _TEST_CHECK_VAL(long, std::numeric_limits<long>::min());
    _TEST_CHECK_VAL(long, std::numeric_limits<long>::max());
    //_TEST_CHECK_VAL(long, std::numeric_limits<long>::signaling_NaN());
    //_TEST_CHECK_VAL(long, std::numeric_limits<long>::quiet_NaN());

    _TEST_CHECK_VAL(unsigned long, 1);
    _TEST_CHECK_VAL(unsigned long, -1);
    _TEST_CHECK_VAL(unsigned long, std::numeric_limits<unsigned long>::min());
    _TEST_CHECK_VAL(unsigned long, std::numeric_limits<unsigned long>::max());

    _TEST_CHECK_VAL(long long, 1);
    _TEST_CHECK_VAL(long long, -1);
    _TEST_CHECK_VAL(long long, std::numeric_limits<long long>::min());
    _TEST_CHECK_VAL(long long, std::numeric_limits<long long>::max());
    //_TEST_CHECK_VAL(long long, std::numeric_limits<long long>::signaling_NaN());
    //_TEST_CHECK_VAL(long long, std::numeric_limits<long long>::quiet_NaN());

    _TEST_CHECK_VAL(unsigned long long, 1);
    _TEST_CHECK_VAL(unsigned long long, -1);
    _TEST_CHECK_VAL(unsigned long long, std::numeric_limits<unsigned long long>::min());
    _TEST_CHECK_VAL(unsigned long long, std::numeric_limits<unsigned long long>::max());

    _TEST_CHECK_VAL(float, 1);
    _TEST_CHECK_VAL(float, -1);
    _TEST_CHECK_VAL(float, std::numeric_limits<float>::min());
    _TEST_CHECK_VAL(float, std::numeric_limits<float>::max());
    _TEST_CHECK_VAL_NOT(float, std::numeric_limits<float>::signaling_NaN());
    _TEST_CHECK_VAL_NOT(float, std::numeric_limits<float>::quiet_NaN());

    _TEST_CHECK_VAL(double, 1);
    _TEST_CHECK_VAL(double, -1);
    _TEST_CHECK_VAL(double, std::numeric_limits<double>::min());
    _TEST_CHECK_VAL(double, std::numeric_limits<double>::max());
    _TEST_CHECK_VAL_NOT(double, std::numeric_limits<double>::signaling_NaN());
    _TEST_CHECK_VAL_NOT(double, std::numeric_limits<double>::quiet_NaN());

    /* const char* str is passed as value, so the pointer is not equal */
    _TEST_CHECK_VAL_NOT(const char*, "1234567890");
    _TEST_CHECK_VAL_NOT(const char*, "abcdefghijklmnopqrstuvwxyz");

    {
        const char* v;
        auto guard = s->Call("Check", std::tie(v), "1234567890");
        ASSERT_TRUE(guard);
        ASSERT_STREQ(v, "1234567890");
    }

    {
        const char* v;
        auto guard = s->Call("Check", std::tie(v), "abcdefghijklmnopqrstuvwxyz");
        ASSERT_TRUE(guard);
        ASSERT_STREQ(v, "abcdefghijklmnopqrstuvwxyz");
    }

    _TEST_CHECK_VAL(std::string, "1234567890");
    _TEST_CHECK_VAL(std::string, "abcdefghijklmnopqrstuvwxyz");

    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}


TEST(xlua, LifeTime) {
    xlua::State* s = xlua::Create(nullptr);

    ASSERT_EQ(LifeTime::s_counter, 0);
    {
        // value
        s->Push(LifeTime());
        ASSERT_EQ(LifeTime::s_counter, 1);
        s->PopTop(1);
        s->Gc();
        ASSERT_EQ(LifeTime::s_counter, 0);
    }

    {
        // ptr
        LifeTime l;
        ASSERT_EQ(LifeTime::s_counter, 1);
        s->Push(&l);
        ASSERT_EQ(LifeTime::s_counter, 1);
        s->PopTop(1);
    }
    ASSERT_EQ(LifeTime::s_counter, 0);

    // lua c upvalue
    {
        LifeTime l;
        std::function<void()> f = [l]() {
            printf("std::function life time:%d==3, %p\n", LifeTime::s_counter, &l);
        };
        ASSERT_EQ(LifeTime::s_counter, 2);

        s->Push(f);
        ASSERT_EQ(LifeTime::s_counter, 3);
        s->Call(std::tie());
        s->Gc();
        ASSERT_EQ(LifeTime::s_counter, 2);
        f = nullptr;
        ASSERT_EQ(LifeTime::s_counter, 1);
    }
    ASSERT_EQ(LifeTime::s_counter, 0);

    {
        int ret = 0;
        int idx = 0;
        LifeTime l;
        s->PushLambda([l, idx]() mutable->int {
            printf("lambda life time:%d==2, %p\n", LifeTime::s_counter, &l);
            return ++idx;
        });
        ASSERT_EQ(LifeTime::s_counter, 2);

        auto f = s->Get<std::function<int()>>(-1);
        ASSERT_EQ(LifeTime::s_counter, 2);  // will obatain a xlua::Function object
                                            // would not copy the lambda object
        s->Call(std::tie(ret));
        ASSERT_EQ(ret, 1);
        s->Gc();
        ASSERT_EQ(LifeTime::s_counter, 2);  // obj is hold by function, will not gc

        ASSERT_EQ(f(), 2);                  // call the lua hold lambda object

        f = nullptr;
        s->Gc();
        ASSERT_EQ(LifeTime::s_counter, 1);  // will release the lambda object
    }

    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

TEST(xlua, LuaCallGuard) {
    xlua::State* s = xlua::Create(nullptr);
    const char* check_func = "function Check(...) return ... end";
    ASSERT_TRUE(s->DoString(check_func, "Check"));

    /* need guard */
    Object* ptr = nullptr;
    Object obj;
    strcpy_s(obj.name, 64, "hello world");

    ASSERT_TRUE(s->Call("Check", std::tie(ptr), obj));
    ASSERT_NE(ptr, &obj);                       // the ptr is refer the lua object
    ASSERT_EQ(s->GetTop(), 0);                  // call guard will clear lua stack
    EXPECT_STREQ(ptr->name, "hello world");     // gc may not immediately
    s->Gc();                                    // gc will release the pushed object
    EXPECT_STRNE(ptr->name, "hello world");
    EXPECT_STREQ(ptr->name, "destructed");      // gc will release the push object

    bool boolean_val;
    const char* str_val;

    XLUA_IF_CALL_SUCC(s->Call("Check", std::tie(boolean_val, str_val), true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 2);
    }

    XLUA_IF_CALL_SUCC(s->Call("Check_not_exist", std::tie(boolean_val, str_val), true, "hello world")) {
    } else {
        ASSERT_EQ(s->GetTop(), 1);
    }

    XLUA_IF_CALL_FAIL(s->Call("Check_not_exist", std::tie(boolean_val, str_val), true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 1);
    }

    XLUA_IF_CALL_FAIL(s->Call("Check", std::tie(boolean_val, str_val), true, "hello world")) {
    } else {
        ASSERT_EQ(s->GetTop(), 2);
    }

    if (auto guard = s->DoString("return ...", "guard", true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 2);
    }

    /* if add '== false' experise, the guard will destrction immediately */
    if (auto guard = s->DoString("return ...", "guard", true, "hello world") == false) {
    } else {
        EXPECT_EQ(s->GetTop(), 0);
    }

    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

TEST(xlua, PushLoadObject) {

}
