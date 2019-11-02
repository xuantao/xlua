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
};

struct Derived : TestObj {
    void print() {

    }
};

XLUA_DECLARE_CLASS(TestObj);
XLUA_DECLARE_CLASS(Derived);
