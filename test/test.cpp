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

    void Startup(xlua::State* s) {
        XCALL_SUCC(s->DoString(script_xlua_op, "op")) {
            op = s->Get<xlua::Function>(-1);
        }
        assert(op.IsValid());

        XCALL_SUCC(s->DoString(script_check, "check")) {
            check = s->Get<xlua::Function>(-1);
        }
        assert(check.IsValid());

        XCALL_SUCC(s->DoString(script_pair, "pairs")) {
            pairs = s->Get<xlua::Function>(-1);
        }
        assert(pairs.IsValid());

        XCALL_SUCC(s->DoString(script_ipair, "ipairs")) {
            ipairs = s->Get<xlua::Function>(-1);
        }
        assert(ipairs.IsValid());

        XCALL_SUCC(s->DoString(script_set_field, "set_field")) {
            set_field = s->Get<xlua::Function>(-1);
        }
        assert(set_field.IsValid());

        XCALL_SUCC(s->DoString(script_get_field, "get_field")) {
            get_field = s->Get<xlua::Function>(-1);
        }
        assert(get_field.IsValid());
    }

    void Clear() {
        op = nullptr;
        check = nullptr;
        pairs = nullptr;
        ipairs = nullptr;
        set_field = nullptr;
        get_field = nullptr;
    }

    xlua::Function op;
    xlua::Function check;
    xlua::Function pairs;
    xlua::Function ipairs;
    xlua::Function set_field;
    xlua::Function get_field;
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

    if (auto guard = s->DoString("return ...", "guard", true, "hello world")) {
        ASSERT_EQ(s->GetTop(), 2);
    }

    /* if add '== false' experise, the guard will destrct immediately */
    if (auto guard = s->DoString("return ...", "guard", true, "hello world") == false) {
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

        XCALL_SUCC(s->DoString("return function (...) return ... end", "")) {
            ASSERT_EQ(s->GetTop(), 1);
            table.SetField("Check");
            ASSERT_EQ(s->GetTop(), 0);  // will set the top value as table field and remove ths stack value
        }

        auto func = table.GetField<xlua::Function>("Check");
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

TEST(xlua, TestUserData) {
    xlua::State* s = xlua::Create(nullptr);

    {
        // load static value
        //auto* vec = s->GetGlobal<std::vector<int>*>("TestMember.s_vector_val");
        //ASSERT_NE(vec, nullptr);
        //vec->push_back(2);
        //vec->push_back(3);
        //s->Push(vec);

        //auto ud = s->Get<xlua::UserData>(-1);
        //s->PopTop(1);
        //ASSERT_EQ(vec, ud.ToPtr());

        //ASSERT_EQ(s->GetTop(), 0);
        //ASSERT_TRUE(ud.IsValid());

        //ud.SetField(1, 100);
        //ASSERT_EQ(ud.GetField<int>(1), 100);

        //s->Push(101);
        //ud.SetField(2); // will pop the top value
        //ASSERT_EQ(s->GetTop(), 0);
        //ASSERT_EQ(ud.GetField<int>(2), 101);

        //auto* vec2 = ud.As<std::vector<int>*>();
        //ASSERT_EQ(vec, vec2);

        //s->Push(ud);
        //auto* vec3 = s->Get<std::vector<int>*>(-1);
        //s->PopTop(1);
        //ASSERT_EQ(vec, vec3);

        //ud = nullptr;
    }

    {
        s->Push(TestMember());
        auto ud = s->Get<xlua::UserData>(-1);
        s->PopTop(1);
        s->Gc();
        ASSERT_TRUE(ud.IsValid());  // ud will cache the lua object

        xlua::Function get_var;
        XCALL_SUCC(s->DoString("return function (obj, name) print(name) return obj[name] end", "GetVar")) {
            get_var = s->Get<xlua::Function>(-1);
            ASSERT_TRUE(ud.IsValid());
        }
        ASSERT_TRUE(get_var.IsValid());
        ASSERT_EQ(s->GetTop(), 0);

        xlua::UserData ud_map;
        ASSERT_TRUE(get_var(std::tie(ud_map), ud, "map_val"));

        auto* ptr = ud.As<TestMember*>();
        auto* mem = ud_map.As<std::map<std::string, TestMember*>*>();
        ASSERT_EQ(&ptr->map_val, mem);  // ud's object member will pass as pointer

        ud_map.SetField("first", ud);
        ASSERT_EQ(ptr->map_val["first"], ptr);

        ud_map.SetField("first", nullptr);
        ASSERT_TRUE(ptr->map_val.empty());

        ptr->m_lua_name__ = 1001;
        int val = ud.GetField<int>("m_lua_name__");
        ASSERT_NE(val, 1001);

        // lua member name has purified
        val = ud.GetField<int>(xlua::internal::PurifyMemberName("m_lua_name__"));
        ASSERT_EQ(val, 1001);

        ud = nullptr;
        s->Gc();
        // after gc, ud_map is an ilegal value, we must carefull about the life time
    }

    ASSERT_EQ(s->GetTop(), 0);
    s->Release();
}
