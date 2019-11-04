#include <iostream>
#include "xlua.h"
#include "export_prepare.h"

struct TestNone;

XLUA_DECLARE_CLASS(TestNone);

struct TestNone {
};

enum class TestEnum {
    kNone,
    kOne,
    kTwo,
};

int main(int argc, char* argv) {
    {
        xlua::State* l = xlua::CreateState(nullptr);

        l->Push(1);
        l->Push(true);
        const char* p = "str";
        l->Push(p);
        l->Push("array");

        bool btype = l->IsType<int, bool, const char*, std::string>(1);
        btype = l->IsType<bool, bool, const char*, const char*>(1);

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
            return i+1;
        };
        l->Push(f);
        auto f2 = l->Load<std::function<int(int)>>(-1);

        const char* t1 = l->Load<const char*>(3);
        //const char* t2 = l->Load<char*>(3);
        xlua::DestoryState(l);


    }
    return 0;
}
