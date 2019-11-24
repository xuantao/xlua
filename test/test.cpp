#include "lua_export.h"
#include "xlua.h"
#include "gtest/gtest.h"

static constexpr const char* kCheckFunc = "function Check(...) return ... end";

struct ScriptOps {
    static constexpr const char* script_xlua_op = R"(
return function (op, obj, ...)
    return xlua[op](obj, ...)
end
)";

    static constexpr const char* script_check = R"(
return function (...)
    return ...
end
)";

    static constexpr const char* script_pair = R"(
return function (obj)
    local total_1 = 0
    for k, v in pairs(obj) do
        print(k, v)
        total_1 = total_1 + v
    end
    return total_1
end
)";

    static constexpr const char* script_ipair = R"(
return function (obj)
    local total_2 = 0;
    for k, v in ipairs(obj) do
        print(k, v)
        total_2 = total_2 + v
    end
    return total_2
end
)";

    static constexpr const char* script_pair_print = R"(
return function (obj)
    for k, v in pairs(obj) do
        print(k, v)
    end
end
)";

    static constexpr const char* script_set_field = R"(
return function (obj, key, value)
    obj[key] = value
end
)";

    static constexpr const char* script_get_field = R"(
return function (obj, key)
    return obj[key]
end
)";

    static constexpr const char* script_call = R"(
return function (obj, key, ...)
    return obj[key](obj, ...)
end
)";

    static constexpr const char* script_dot_call = R"(
return function (obj, key, ...)
    return obj[key](...)
end
)";

    void Startup(xlua::State* s) {
        EXPECT_TRUE(s->DoString(script_xlua_op, "op", std::tie(op)));
        EXPECT_TRUE(s->DoString(script_check, "check", std::tie(check)));
        EXPECT_TRUE(s->DoString(script_pair, "pairs", std::tie(pairs)));
        EXPECT_TRUE(s->DoString(script_ipair, "ipairs", std::tie(ipairs)));
        EXPECT_TRUE(s->DoString(script_pair_print, "pairs_print", std::tie(pairs_print)));
        EXPECT_TRUE(s->DoString(script_set_field, "set_field", std::tie(set_field)));
        EXPECT_TRUE(s->DoString(script_get_field, "get_field", std::tie(get_field)));
        EXPECT_TRUE(s->DoString(script_call, "call", std::tie(call)));
        EXPECT_TRUE(s->DoString(script_dot_call, "dot_call", std::tie(dot_call)));
    }

    void Clear() {
        op = nullptr;
        check = nullptr;
        pairs = nullptr;
        ipairs = nullptr;
        pairs_print = nullptr;
        set_field = nullptr;
        get_field = nullptr;
        call = nullptr;
        dot_call = nullptr;
    }

    xlua::Function op;
    xlua::Function check;
    xlua::Function pairs;
    xlua::Function ipairs;
    xlua::Function pairs_print;
    xlua::Function set_field;
    xlua::Function get_field;
    xlua::Function call;
    xlua::Function dot_call;
};

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

    _TEST_CHECK_VAL(unsigned short, 1);
    _TEST_CHECK_VAL(unsigned short, (unsigned short)-1);
    _TEST_CHECK_VAL(unsigned short, std::numeric_limits<unsigned short>::min());
    _TEST_CHECK_VAL(unsigned short, std::numeric_limits<unsigned short>::max());

    _TEST_CHECK_VAL(int, 1);
    _TEST_CHECK_VAL(int, (int)-1);
    _TEST_CHECK_VAL(int, std::numeric_limits<int>::min());
    _TEST_CHECK_VAL(int, std::numeric_limits<int>::max());

    _TEST_CHECK_VAL(unsigned int, 1);
    _TEST_CHECK_VAL(unsigned int, (unsigned int)-1);
    _TEST_CHECK_VAL(unsigned int, std::numeric_limits<unsigned int>::min());
    _TEST_CHECK_VAL(unsigned int, std::numeric_limits<unsigned int>::max());

    _TEST_CHECK_VAL(long, 1);
    _TEST_CHECK_VAL(long, -1);
    _TEST_CHECK_VAL(long, std::numeric_limits<long>::min());
    _TEST_CHECK_VAL(long, std::numeric_limits<long>::max());

    _TEST_CHECK_VAL(unsigned long, 1);
    _TEST_CHECK_VAL(unsigned long, -1);
    _TEST_CHECK_VAL(unsigned long, std::numeric_limits<unsigned long>::min());
    _TEST_CHECK_VAL(unsigned long, std::numeric_limits<unsigned long>::max());

    _TEST_CHECK_VAL(long long, 1);
    _TEST_CHECK_VAL(long long, -1);
    _TEST_CHECK_VAL(long long, std::numeric_limits<long long>::min());
    _TEST_CHECK_VAL(long long, std::numeric_limits<long long>::max());

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
    Doodad obj;
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

    XCALL_SUCC(s->Call("Check", std::tie(boolean_val, str_val), true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 2);
    }

    XCALL_SUCC(s->Call("Check_not_exist", std::tie(boolean_val, str_val), true, "hello world")) {
    } else {
        ASSERT_EQ(s->GetTop(), 1);
    }

    XCALL_FAIL(s->Call("Check_not_exist", std::tie(boolean_val, str_val), true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 1);
    }

    XCALL_FAIL(s->Call("Check", std::tie(boolean_val, str_val), true, "hello world")) {
    } else {
        ASSERT_EQ(s->GetTop(), 2);
    }

    if (auto guard = s->DoString("return ...", "guard", std::tie(boolean_val, str_val), true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 2);
    }

    /* if add '== false' experise, the guard will destrct immediately */
    if (auto guard = s->DoString("return ...", "guard", std::tie(boolean_val, str_val), true, "hello world") == false) {
    } else {
        EXPECT_EQ(s->GetTop(), 0);
    }

    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

TEST(xlua, TestTable) {
    xlua::State* s = xlua::Create(nullptr);

    {
        xlua::Table table;
        ASSERT_EQ(table.begin(), table.end());

        s->NewTable();
        table = s->Get<xlua::Variant>(-1).ToTable();
        ASSERT_EQ(table.begin(), table.end());

        // assign table field
        for (int i = 0; i < 10; ++i) {
            s->SetField(-1, i + 1, i + 1);
        }
        ASSERT_EQ(s->GetTop(), 1);
        ASSERT_NE(table.begin(), table.end());
        s->PopTop(1);

        // check table value
        int idx = 1;
        for (auto beg = table.begin(), end = table.end(); beg != end; ++beg, ++idx) {
            ASSERT_EQ(beg->first.ToInt(), idx);
            ASSERT_EQ(beg->second.ToInt(), idx);
        }
        idx = 1;
        for (auto pair : table) {
            ASSERT_EQ(pair.first.ToInt(), idx);
            ASSERT_EQ(pair.second.ToInt(), idx);
            ++idx;
        }

        // clear table
        for (int i = 0; i < 10; ++i) {
            table.SetField(i+1, nullptr);
        }
        ASSERT_EQ(s->GetTop(), 0);
        ASSERT_EQ(table.begin(), table.end());

        TestMember member;
        s->Push(member);
        table.SetField("obj_value");
        ASSERT_EQ(s->GetTop(), 0);

        auto var_obj_value = table.GetField<xlua::Variant>("obj_value");
        auto* obj_value_ptr = table.GetField<TestMember*>("obj_value");
        ASSERT_EQ(obj_value_ptr, var_obj_value.ToObj<TestMember*>());
        ASSERT_EQ(s->GetTop(), 0);

        table.LoadField("obj_value");
        ASSERT_EQ(obj_value_ptr, s->Get<TestMember*>(-1));
        s->PopTop(1);
        ASSERT_EQ(s->GetTop(), 0);

        table.SetField("obj_ptr", &member);
        ASSERT_EQ(s->GetTop(), 0);

        auto obj_ptr_var = table.GetField<xlua::Variant>("obj_ptr");
        auto* obj_ptr_ptr = table.GetField<TestMember*>("obj_ptr");
        ASSERT_EQ(obj_ptr_ptr, obj_ptr_var.ToObj<TestMember*>());
        ASSERT_EQ(obj_ptr_ptr, &member);
        ASSERT_EQ(s->GetTop(), 0);

        table.LoadField("obj_ptr");
        ASSERT_EQ(obj_ptr_ptr, s->Get<TestMember*>(-1));
        s->PopTop(1);
        ASSERT_EQ(s->GetTop(), 0);

        xlua::Function func;
        XCALL_SUCC(s->DoString("return function (...) return ... end", "", std::tie(func))) {
            ASSERT_EQ(s->GetTop(), 1);
            table.SetField("Check");
            ASSERT_EQ(s->GetTop(), 0);  // will set the top value as table field and remove ths stack value
        }
        ASSERT_TRUE(func.IsValid());

        int i = 0;
        table.DotCall("Check", std::tie(i), 1);
        ASSERT_EQ(i, 1);

        xlua::Table ret;
        table.Call("Check", std::tie(ret));
        ASSERT_EQ(table, ret);
        //ASSERT_NE(table, ret);  // here is a new table ref to the same table
    }

    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

/* vector list map unordered_map */
TEST(xlua, TestCollection) {
    xlua::State* s = xlua::Create(nullptr);

    ScriptOps ops;
    ops.Startup(s);

    //vector
    {
        s->Push(std::vector<int>());
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);

        auto* ptr = ud.As<std::vector<int>*>();

        // invalid set key
        //ud.SetField("str", 1);    // this call will crash, will directly trigger lua_error
                                    // invalid key
        printf("set field, invalid key\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "str", 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "-1", 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "0", 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "2", 1));

        // push back key
        ud.SetField(ptr->size() + 1, 1);    // index == size + 1, equal push_back
        ASSERT_EQ(ptr->size(), 1);

        int ret = ud.GetField<int>(1);
        ASSERT_EQ(ret, 1);

        // invalid value
        printf("set field, invalid value\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, 1, false));
        // invalid get key
        printf("get field, invalid key\n");
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "str"));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "-1"));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "0"));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "2"));

        EXPECT_TRUE(ops.op(std::tie(), "Clear", ud));
        ASSERT_EQ(ptr->size(), 0);

        EXPECT_TRUE(ops.op(std::tie(), "Insert", ptr, 1, 100));
        EXPECT_TRUE(ops.op(std::tie(), "Insert", ptr, 2, 200));
        ASSERT_EQ(ptr->size(), 2);
        ASSERT_EQ(ptr->at(0), 100);
        ASSERT_EQ(ptr->at(1), 200);

        printf("xlua.Insert, invalid key\n");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, "str", 200));
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 0, 100));
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 4, 200));
        printf("insert, invalid value\n");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 3, true));

        printf("xlua.Remove, invalid key\n");
        EXPECT_FALSE(ops.op(std::tie(), "Remove", ud, "str"));
        EXPECT_FALSE(ops.op(std::tie(), "Remove", ud, 0));
        EXPECT_FALSE(ops.op(std::tie(), "Remove", ud, 4));

        EXPECT_TRUE(ops.op(std::tie(), "Remove", ud, 1));
        EXPECT_TRUE(ops.op(std::tie(), "Remove", ud, 1));
        ASSERT_EQ(ptr->size(), 0);

        ud.SetField(ptr->size() + 1, 1);
        ud.SetField(ptr->size() + 1, 1);
        ud.SetField(ptr->size() + 1, 1);

        ret = 0;
        EXPECT_TRUE(ops.pairs(std::tie(ret), ud));
        EXPECT_EQ(ret, 3);

        // userdata can not override ipairs, what a pity?
        //ret = 0;
        //EXPECT_TRUE(ops.ipairs(std::tie(ret), ud));
        //EXPECT_EQ(ret, 3);
    }

    //list
    {
        s->Push(std::list<int>());
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);

        auto* ptr = ud.As<std::list<int>*>();

        // invalid set key
        //ud.SetField("str", 1);    // this call will crash, will directly trigger lua_error
                                    // invalid key
        printf("set field, invalid key\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "str", 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "-1", 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "0", 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "2", 1));

        // push back key
        ud.SetField(ptr->size() + 1, 1);    // index == size + 1, equal push_back
        ASSERT_EQ(ptr->size(), 1);

        int ret = ud.GetField<int>(1);
        ASSERT_EQ(ret, 1);

        // invalid value
        printf("set field, invalid value\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, 1, false));
        // invalid get key
        printf("get field, invalid key\n");
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "str"));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "-1"));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "0"));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, "2"));

        EXPECT_TRUE(ops.op(std::tie(), "Clear", ud));
        ASSERT_EQ(ptr->size(), 0);

        EXPECT_TRUE(ops.op(std::tie(), "Insert", ptr, 1, 100));
        EXPECT_TRUE(ops.op(std::tie(), "Insert", ptr, 2, 200));
        ASSERT_EQ(ptr->size(), 2);
        ASSERT_EQ(ptr->front(), 100);
        ASSERT_EQ(ptr->back(), 200);

        printf("xlua.Insert, invalid key\n");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, "str", 200));
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 0, 100));
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 4, 200));
        printf("insert, invalid value\n");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 3, true));

        printf("xlua.Remove, invalid key\n");
        EXPECT_FALSE(ops.op(std::tie(), "Remove", ud, "str"));
        EXPECT_FALSE(ops.op(std::tie(), "Remove", ud, 0));
        EXPECT_FALSE(ops.op(std::tie(), "Remove", ud, 4));

        EXPECT_TRUE(ops.op(std::tie(), "Remove", ud, 1));
        EXPECT_TRUE(ops.op(std::tie(), "Remove", ud, 1));
        ASSERT_EQ(ptr->size(), 0);

        ud.SetField(ptr->size() + 1, 1);
        ud.SetField(ptr->size() + 1, 1);
        ud.SetField(ptr->size() + 1, 1);

        ret = 0;
        EXPECT_TRUE(ops.pairs(std::tie(ret), ud));
        EXPECT_EQ(ret, 3);

        // userdata can not override ipairs, what a pity?
        //ret = 0;
        //EXPECT_TRUE(ops.ipairs(std::tie(ret), ud));
        //EXPECT_EQ(ret, 3);
    }

    // map
    {
        s->Push(std::map<std::string, int>());
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);
        ASSERT_EQ(s->GetTop(), 0);
        ASSERT_TRUE(ud.IsValid());

        auto* ptr = ud.As<std::map<std::string, int>*>();
        ASSERT_TRUE(ptr);

        printf("set field, invalid key\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, false, 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, 1, 1));

        printf("set field, invalid value\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "one", "1"));

        s->Push(1);
        ud.SetField("one");
        ud.SetField("two", 2);
        ASSERT_EQ(ptr->size(), 2);
        ASSERT_EQ(s->GetTop(), 0);

        int ret = 0;
        printf("get field, invalid key\n");
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, false));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, 1));

        EXPECT_TRUE(ops.get_field(std::tie(ret), ptr, "two"));
        ASSERT_EQ(ret, 2);

        EXPECT_TRUE(ops.get_field(std::tie(ret), ptr, "one"));
        ASSERT_EQ(ret, 1);

        ret = ud.GetField<int>("one");
        ASSERT_EQ(ret, 1);
        ret = ud.GetField<int>("two");
        ASSERT_EQ(ret, 2);

        ud.LoadField("one");
        ASSERT_EQ(s->Get<int>(-1), 1);
        ud.LoadField("two");
        ASSERT_EQ(s->Get<int>(-1), 2);
        ud.LoadField("three");
        ASSERT_TRUE(s->IsNil(-1));
        s->PopTop(3);

        printf("xlua.Insert, invalid key");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, false, 1));
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 1, 1));

        printf("xlua.Insert, invalid value");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, "three", "three"));

        EXPECT_TRUE(ops.op(std::tie(), "Insert", ud, "three", 3));
        ASSERT_EQ(ptr->find("three")->second, 3);
        ASSERT_EQ(ptr->size(), 3);

        printf("xlua.Remove, invalid key");
        EXPECT_FALSE(ops.op(std::tie(ret), "Remove", ud, false));
        EXPECT_FALSE(ops.op(std::tie(ret), "Remove", ud, 1));

        ret = 0;
        EXPECT_TRUE(ops.pairs(std::tie(ret), ud));
        ASSERT_EQ(ret, 6);

        EXPECT_TRUE(ops.op(std::tie(ret), "Remove", ud, "one"));
        EXPECT_TRUE(ops.op(std::tie(ret), "Remove", ud, "two"));
        EXPECT_TRUE(ops.op(std::tie(ret), "Remove", ud, "three"));

        ASSERT_EQ(ptr->size(), 0);
    }

    // unordered_map
    {
        s->Push(std::unordered_map<std::string, int>());
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);
        ASSERT_EQ(s->GetTop(), 0);
        ASSERT_TRUE(ud.IsValid());

        auto* ptr = ud.As<std::unordered_map<std::string, int>*>();
        ASSERT_TRUE(ptr);

        printf("set field, invalid key\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, false, 1));
        EXPECT_FALSE(ops.set_field(std::tie(), ud, 1, 1));

        printf("set field, invalid value\n");
        EXPECT_FALSE(ops.set_field(std::tie(), ud, "one", "1"));

        s->Push(1);
        ud.SetField("one");
        ud.SetField("two", 2);
        ASSERT_EQ(ptr->size(), 2);
        ASSERT_EQ(s->GetTop(), 0);

        int ret = 0;
        printf("get field, invalid key\n");
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, false));
        EXPECT_FALSE(ops.get_field(std::tie(ret), ud, 1));

        EXPECT_TRUE(ops.get_field(std::tie(ret), ptr, "two"));
        ASSERT_EQ(ret, 2);

        EXPECT_TRUE(ops.get_field(std::tie(ret), ptr, "one"));
        ASSERT_EQ(ret, 1);

        ret = ud.GetField<int>("one");
        ASSERT_EQ(ret, 1);

        ret = ud.GetField<int>("two");
        ASSERT_EQ(ret, 2);

        ud.LoadField("one");
        ASSERT_EQ(s->Get<int>(-1), 1);
        ud.LoadField("two");
        ASSERT_EQ(s->Get<int>(-1), 2);
        ud.LoadField("three");
        ASSERT_TRUE(s->IsNil(-1));
        s->PopTop(3);

        printf("xlua.Insert, invalid key");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, false, 1));
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, 1, 1));

        printf("xlua.Insert, invalid value");
        EXPECT_FALSE(ops.op(std::tie(), "Insert", ud, "three", "three"));

        EXPECT_TRUE(ops.op(std::tie(), "Insert", ud, "three", 3));
        ASSERT_EQ(ptr->find("three")->second, 3);
        ASSERT_EQ(ptr->size(), 3);

        printf("xlua.Remove, invalid key");
        EXPECT_FALSE(ops.op(std::tie(ret), "Remove", ud, false));
        EXPECT_FALSE(ops.op(std::tie(ret), "Remove", ud, 1));

        ret = 0;
        EXPECT_TRUE(ops.pairs(std::tie(ret), ud));
        ASSERT_EQ(ret, 6);

        EXPECT_TRUE(ops.op(std::tie(ret), "Remove", ud, "one"));
        EXPECT_TRUE(ops.op(std::tie(ret), "Remove", ud, "two"));
        EXPECT_TRUE(ops.op(std::tie(ret), "Remove", ud, "three"));

        ASSERT_EQ(ptr->size(), 0);
    }

    ops.Clear();
    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

static void DoTestUserData(xlua::State* s, xlua::UserData ud, ScriptOps& ops) {
    bool b_val;
    int i_val;
    long l_val;
    const char* str_val;

    // set field
    ASSERT_TRUE(ops.set_field(std::tie(), ud, "boolean_val", true));
    ASSERT_TRUE(ops.set_field(std::tie(), ud, "int_val", 10001));
    ASSERT_TRUE(ops.set_field(std::tie(), ud, "long_val", 10002));
    ASSERT_TRUE(ops.set_field(std::tie(), ud, "name_val", "TestMember"));
    ASSERT_TRUE(ops.set_field(std::tie(), ud,
        xlua::internal::PurifyMemberName("m_lua_name__"), 10003));

    // member name has been purified
    ASSERT_FALSE(ops.set_field(std::tie(), ud, "m_lua_name__", 10003));
    // member is read only
    ASSERT_FALSE(ops.set_field(std::tie(), ud, "read_only", 2005));

    // get field
    ASSERT_TRUE(ops.get_field(std::tie(b_val), ud, "boolean_val"));
    EXPECT_EQ(b_val, true);
    ASSERT_TRUE(ops.get_field(std::tie(i_val), ud, "int_val"));
    EXPECT_EQ(i_val, 10001);
    ASSERT_TRUE(ops.get_field(std::tie(l_val), ud, "long_val"));
    EXPECT_EQ(l_val, 10002);
    ASSERT_TRUE(ops.get_field(std::tie(str_val), ud, "name_val"));
    EXPECT_STREQ(str_val, "TestMember");
    ASSERT_TRUE(ops.get_field(std::tie(i_val), ud, xlua::internal::PurifyMemberName("m_lua_name__")));
    EXPECT_EQ(i_val, 10003);
    ASSERT_TRUE(ops.get_field(std::tie(i_val), ud, "read_only"));
    EXPECT_EQ(i_val, 1111);
    // member name has been purified
    ASSERT_FALSE(ops.get_field(std::tie(i_val), ud, "m_lua_name__"));

    ASSERT_TRUE(ops.call(std::tie(), ud, "Test", "test_call"));
    ASSERT_FALSE(ops.dot_call(std::tie(), ud, "Test", "test_call"));

    ASSERT_TRUE(ops.pairs_print(std::tie(), ud));
}

TEST(xlua, TestUserData) {
    xlua::State* s = xlua::Create(nullptr);

    ScriptOps ops;
    ops.Startup(s);
    {
        TestMember obj;
        obj.read_only = 1111;
        s->Push(obj);
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);

        DoTestUserData(s, ud, ops);
    }

    {
        TestMember obj;
        obj.read_only = 1111;
        s->Push(&obj);
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);

        DoTestUserData(s, ud, ops);
    }

    ops.Clear();
    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

TEST(xlua, TestStaticMember) {
    xlua::State* s = xlua::Create(nullptr);
    ScriptOps ops;
    ops.Startup(s);

    {
        auto table = s->GetGlobal<xlua::Table>("TestMember");
        TestMember::s_read_only = 1111;
        bool b_val;
        int i_val;
        long l_val;
        const char* str_val;

        // set field
        ASSERT_TRUE(ops.set_field(std::tie(), table, "s_boolean_val", true));
        ASSERT_TRUE(ops.set_field(std::tie(), table, "s_int_val", 10001));
        ASSERT_TRUE(ops.set_field(std::tie(), table, "s_long_val", 10002));
        ASSERT_TRUE(ops.set_field(std::tie(), table, "s_name_val", "TestMember"));
        ASSERT_TRUE(ops.set_field(std::tie(), table,
            xlua::internal::PurifyMemberName("s_lua_name__"), 10003));

        // member name has been purified
        ASSERT_FALSE(ops.set_field(std::tie(), table, "s_lua_name__", 10003));
        // member is read only
        ASSERT_FALSE(ops.set_field(std::tie(), table, "s_read_only", 2005));

        // get field
        ASSERT_TRUE(ops.get_field(std::tie(b_val), table, "s_boolean_val"));
        EXPECT_EQ(b_val, true);
        ASSERT_TRUE(ops.get_field(std::tie(i_val), table, "s_int_val"));
        EXPECT_EQ(i_val, 10001);
        ASSERT_TRUE(ops.get_field(std::tie(l_val), table, "s_long_val"));
        EXPECT_EQ(l_val, 10002);
        ASSERT_TRUE(ops.get_field(std::tie(str_val), table, "s_name_val"));
        EXPECT_STREQ(str_val, "TestMember");
        ASSERT_TRUE(ops.get_field(std::tie(i_val), table, xlua::internal::PurifyMemberName("s_lua_name__")));
        EXPECT_EQ(i_val, 10003);
        ASSERT_TRUE(ops.get_field(std::tie(i_val), table, "s_read_only"));
        EXPECT_EQ(i_val, 1111);
        // member name has been purified
        ASSERT_FALSE(ops.get_field(std::tie(i_val), table, "s_lua_name__"));

        ASSERT_FALSE(ops.call(std::tie(), table, "sTest", "test_call"));
        ASSERT_TRUE(ops.dot_call(std::tie(), table, "sTest", "test_call"));

        ASSERT_TRUE(ops.pairs_print(std::tie(), table));
    }

    ops.Clear();
    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

TEST(xlua, TestXluaWeakObj) {
    xlua::State* s = xlua::Create(nullptr);
    ScriptOps ops;
    ops.Startup(s);

    {
        Doodad doodad;
        Character character;
        Object* doodad_ptr = &doodad;
        Object* chatacter_ptr = &character;

        doodad.id_ = 1;
        doodad.drop_id = 1001;
        character.id_ = 2;
        character.hp = 1000;

        xlua::UserData doodad_ud;
        xlua::UserData character_ud;
        ASSERT_TRUE(ops.check(std::tie(doodad_ud), &doodad));
        ASSERT_TRUE(doodad_ud.IsValid());
        ASSERT_TRUE(ops.check(std::tie(character_ud), &character));
        ASSERT_TRUE(character_ud.IsValid());

        int int_ret;
        ASSERT_TRUE(ops.call(std::tie(int_ret), static_cast<Object*>(&doodad), "Update", 2));
        ASSERT_EQ(int_ret, 2);
        ASSERT_TRUE(ops.call(std::tie(int_ret), static_cast<Object*>(&character), "Update", 3));
        ASSERT_EQ(int_ret, 3);

        // dot call a object member function will failed
        EXPECT_FALSE(ops.dot_call(std::tie(int_ret), static_cast<Object*>(&doodad), "Update", 2));
        EXPECT_FALSE(ops.dot_call(std::tie(int_ret), static_cast<Object*>(&character), "Update", 3));

        bool boolean_ret = false;
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", doodad_ud));
        ASSERT_TRUE(boolean_ret);

        boolean_ret = false;
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", character_ud));
        ASSERT_TRUE(boolean_ret);

        ASSERT_EQ(doodad_ptr, doodad_ud.As<Doodad*>());
        ASSERT_EQ(chatacter_ptr, character_ud.As<Character*>());

        xlua::FreeIndex(doodad);
        xlua::FreeIndex(character);
        xlua::FreeIndex(&doodad);
        xlua::FreeIndex(&character);

        /* the user data is still valid, but the reference object is not valid */
        EXPECT_TRUE(doodad_ud.IsValid());
        EXPECT_TRUE(character_ud.IsValid());

        boolean_ret = false;
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", doodad_ud));
        ASSERT_FALSE(boolean_ret);

        boolean_ret = false;
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", character_ud));
        ASSERT_FALSE(boolean_ret);

        ASSERT_FALSE(ops.call(std::tie(int_ret), doodad_ud, "Update", 2));
        ASSERT_FALSE(ops.call(std::tie(int_ret), character_ud, "Update", 3));

        ASSERT_EQ(nullptr, doodad_ud.As<Doodad*>());
        ASSERT_EQ(nullptr, character_ud.As<Character*>());

        xlua::UserData doodad_ud_2;
        xlua::UserData character_ud_2;
        ASSERT_TRUE(ops.check(std::tie(doodad_ud_2), &doodad));
        ASSERT_TRUE(doodad_ud_2.IsValid());
        ASSERT_TRUE(ops.check(std::tie(character_ud_2), &character));
        ASSERT_TRUE(character_ud_2.IsValid());

        ASSERT_TRUE(ops.check(std::tie(doodad_ud_2), static_cast<Object*>(&doodad)));
        ASSERT_TRUE(doodad_ud_2.IsValid());
        ASSERT_TRUE(ops.check(std::tie(character_ud_2), static_cast<Object*>(&character)));
        ASSERT_TRUE(character_ud_2.IsValid());

        ASSERT_NE(doodad_ud, doodad_ud_2);
        ASSERT_NE(character_ud, character_ud_2);

        ASSERT_TRUE(ops.call(std::tie(int_ret), doodad_ud_2, "Update", 2));
        ASSERT_EQ(int_ret, 2);
        ASSERT_TRUE(ops.call(std::tie(int_ret), character_ud_2, "Update", 3));
        ASSERT_EQ(int_ret, 3);

        ASSERT_EQ(doodad_ptr, doodad_ud_2.As<Doodad*>());
        ASSERT_EQ(chatacter_ptr, character_ud_2.As<Character*>());

        ASSERT_FALSE(ops.pairs_print(std::tie(), doodad_ud));
        ASSERT_FALSE(ops.pairs_print(std::tie(), character_ud));
        ASSERT_TRUE(ops.pairs_print(std::tie(), doodad_ud_2));
        ASSERT_TRUE(ops.pairs_print(std::tie(), character_ud_2));
        ASSERT_TRUE(ops.pairs_print(std::tie(), doodad));
        ASSERT_TRUE(ops.pairs_print(std::tie(), character));
    }

    ops.Clear();
    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

TEST(xlua, TestExtWeakObj) {
    xlua::State* s = xlua::Create(nullptr);
    ScriptOps ops;
    ops.Startup(s);

    /* some ud feature */
    {
        CircleCollider circle;
        CapsuleCollider capsule;

        s->Push(&capsule);
        EXPECT_STREQ(s->GetTypeName(-1), "CapsuleCollider");
        s->PopTop(1);

        WeakObjArray::Instance().FreeIndex(&capsule);

        s->Push(&circle);
        EXPECT_STREQ(s->GetTypeName(-1), "CircleCollider");
        s->PopTop(1);
        WeakObjArray::Instance().FreeIndex(&circle);

        /* push same weak object again, the env will update the weak object type info */
        s->Push(static_cast<Collider*>(&capsule));
        EXPECT_STREQ(s->GetTypeName(-1), "Collider");
        s->Push(static_cast<CircleCollider*>(&capsule));
        s->PopTop(1);
        EXPECT_STREQ(s->GetTypeName(-1), "CircleCollider");
        s->Push(static_cast<CapsuleCollider*>(&capsule));
        s->PopTop(1);
        EXPECT_STREQ(s->GetTypeName(-1), "CapsuleCollider");
        s->PopTop(1);
        WeakObjArray::Instance().FreeIndex(&capsule);

        /* push same weak object, the env will record the derived weak object type info */
        s->Push(static_cast<CapsuleCollider*>(&capsule));
        EXPECT_STREQ(s->GetTypeName(-1), "CapsuleCollider");
        s->Push(static_cast<CircleCollider*>(&capsule));
        EXPECT_STREQ(s->GetTypeName(-1), "CapsuleCollider");
        s->Push(static_cast<Collider*>(&capsule));
        EXPECT_STREQ(s->GetTypeName(-1), "CapsuleCollider");
        s->PopTop(1);
        s->PopTop(1);
        s->PopTop(1);
    }

    {
        BoxCollider box;
        CircleCollider circle;
        Collider* box_ptr = &box;
        Collider* circle_ptr = &circle;

        xlua::UserData box_ud;
        xlua::UserData circle_ud;
        ASSERT_TRUE(ops.check(std::tie(box_ud), &box));
        ASSERT_TRUE(box_ud.IsValid());
        ASSERT_TRUE(ops.check(std::tie(circle_ud), &circle));
        ASSERT_TRUE(circle_ud.IsValid());

        bool boolean_ret = false;
        ASSERT_TRUE(ops.call(std::tie(boolean_ret), static_cast<Collider*>(&box), "RayCast", Ray()));
        ASSERT_EQ(boolean_ret, true);
        ASSERT_TRUE(ops.call(std::tie(boolean_ret), static_cast<Collider*>(&circle), "RayCast", Ray()));
        ASSERT_EQ(boolean_ret, true);

        // dot call a object member function will failed
        EXPECT_FALSE(ops.dot_call(std::tie(boolean_ret), static_cast<Collider*>(&box), "RayCast", Ray()));
        EXPECT_FALSE(ops.dot_call(std::tie(boolean_ret), static_cast<Collider*>(&circle), "RayCast", Ray()));

        // the box & circle has push to lua before
        // the lua env has reacord the obj' type info
        const char* str_val = nullptr;
        ASSERT_TRUE(ops.op(std::tie(str_val), "Type", static_cast<Collider*>(&box)));
        EXPECT_STREQ(str_val, "BoxCollider");
        ASSERT_TRUE(ops.op(std::tie(str_val), "Type", static_cast<Collider*>(&circle)));
        EXPECT_STREQ(str_val, "CircleCollider");

        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", box_ud));
        ASSERT_TRUE(boolean_ret);
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", circle_ud));
        ASSERT_TRUE(boolean_ret);

        ASSERT_EQ(box_ptr, box_ud.As<Collider*>());
        ASSERT_EQ(circle_ptr, circle_ud.As<Collider*>());

        WeakObjArray::Instance().FreeIndex(box_ptr);
        WeakObjArray::Instance().FreeIndex(circle_ptr);

        /* the user data is still valid, but the reference object is not valid */
        EXPECT_TRUE(box_ud.IsValid());
        EXPECT_TRUE(circle_ud.IsValid());

        boolean_ret = false;
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", box_ud));
        ASSERT_FALSE(boolean_ret);
        EXPECT_TRUE(ops.op(std::tie(boolean_ret), "IsValid", circle_ud));
        ASSERT_FALSE(boolean_ret);

        ASSERT_FALSE(ops.call(std::tie(boolean_ret), box_ud, "RayCast", Ray()));
        ASSERT_FALSE(ops.call(std::tie(boolean_ret), circle_ud, "RayCast", Ray()));

        ASSERT_EQ(nullptr, box_ud.As<Collider*>());
        ASSERT_EQ(nullptr, circle_ud.As<Collider*>());

        xlua::UserData box_ud_2;
        xlua::UserData circle_ud_2;
        ASSERT_TRUE(ops.check(std::tie(box_ud_2), box_ptr));
        ASSERT_TRUE(box_ud_2.IsValid());
        ASSERT_NE(box_ud, box_ud_2);

        ASSERT_TRUE(ops.check(std::tie(circle_ud_2), circle_ptr));
        ASSERT_TRUE(circle_ud_2.IsValid());
        ASSERT_NE(circle_ud, circle_ud_2);

        ASSERT_TRUE(ops.call(std::tie(boolean_ret), box_ud_2, "RayCast", Ray()));
        ASSERT_TRUE(ops.call(std::tie(boolean_ret), circle_ud_2, "RayCast", Ray()));

        ASSERT_EQ(box_ptr, box_ud_2.As<Collider*>());
        ASSERT_EQ(circle_ptr, circle_ud_2.As<Collider*>());

        ASSERT_FALSE(ops.pairs_print(std::tie(), box_ud));
        ASSERT_TRUE(ops.pairs_print(std::tie(), box_ud_2));

        ASSERT_FALSE(ops.pairs_print(std::tie(), circle_ud));
        ASSERT_TRUE(ops.pairs_print(std::tie(), circle_ud_2));

        WeakObjArray::Instance().FreeIndex(box_ptr);
        WeakObjArray::Instance().FreeIndex(circle_ptr);

        WeakObjPtr<BoxCollider> weak_ref_1 = &box;
        WeakObjPtr<CircleCollider> weak_ref_2 = &circle;
        ASSERT_TRUE(ops.pairs_print(std::tie(), weak_ref_1));
        ASSERT_TRUE(ops.pairs_print(std::tie(), weak_ref_2));

        weak_ref_1 = nullptr;
        weak_ref_2 = nullptr;
        ASSERT_FALSE(ops.pairs_print(std::tie(), weak_ref_1));
        ASSERT_FALSE(ops.pairs_print(std::tie(), weak_ref_2));
    }

    ops.Clear();
    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}

