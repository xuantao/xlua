#pragma once
#include "xlua_def.h"
#include <stdio.h>

struct TestObj {
    TestObj() { printf("TestObj()\n"); }
    TestObj(const TestObj&) { printf("TestObj(const TestObj&)\n"); }
    TestObj(TestObj&&) { printf("TestObj(TestObj&&)\n"); }
    ~TestObj() { printf("~TestObj()\n"); }

    void test() {
    }

    int a;
};

struct Derived : TestObj {
    void print() {

    }

    char szName[64];

    static int sIdx;
};

//XLUA_DECLARE_CLASS(TestObj);
//XLUA_DECLARE_CLASS(Derived);

void test_export();
