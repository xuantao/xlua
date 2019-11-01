#include <iostream>
#include "xlua.h"
#include "export_prepare.h"

struct TestNone;

XLUA_DECLARE_CLASS(TestNone);

struct TestNone {
};


int main(int argc, char* argv) {
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

    TestObj obj;
    l->Push(obj);
    l->Push(&obj);

    const char* t1 = l->Load<const char*>(3);
    //const char* t2 = l->Load<char*>(3);

    return 0;
}
