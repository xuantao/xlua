#pragma once
#include "xlua_def.h"
#include <stdio.h>

struct TestObj {
    TestObj() { printf("TestObj()\n"); }
    TestObj(const TestObj&) { printf("TestObj(const TestObj&)\n"); }
    TestObj(TestObj&&) { printf("TestObj(TestObj&&)\n"); }
    ~TestObj() { printf("~TestObj()\n"); }

    void test() {
        printf("TestObj::test called\n");
    }

    int a;
};

struct Derived : TestObj {
    void print() {
        printf("Derived::print called\n");
    }

    char szName[64];

    static const char* StaticCall(Derived& d) {
        printf("static const char* StaticCall(Derived& d) called\n");
        return "static call";
    }

    static int sIdx;
};

void test_export();
