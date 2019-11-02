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

        char buf[] = "normal array";
        l->Push(buf);

        int i;
        bool b;
        std::string s;
        l->LoadMul(1, std::tie(i, b, s));

        //TestObj obj;
        l->Push(TestObj());
        //l->Push(&obj);
        l->Push(TestEnum::kTwo);
        TestEnum te_1 = l->Load<TestEnum>(-1);

        const char* t1 = l->Load<const char*>(3);
        //const char* t2 = l->Load<char*>(3);
        xlua::DestoryState(l);
    }
    return 0;
}
