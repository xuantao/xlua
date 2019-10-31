#include <iostream>
#include "xlua.h"

int main(int argc, char* argv) {
    xlua::State* l = xlua::CreateState(nullptr);

    l->Push(1);
    l->Push(true);
    const char* p = "str";
    l->Push(p);

    int i;
    bool b;
    std::string s;
    l->LoadMul(1, std::tie(i, b, s));

    const char* t1 = l->Load<const char*>(3);
    //const char* t2 = l->Load<char*>(3);

    return 0;
}
