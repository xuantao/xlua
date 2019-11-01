#pragma once
#include "xlua_def.h"

struct TestObj {
    void test() {
    }
};

struct Derived : TestObj {
    void print() {

    }
};

XLUA_DECLARE_CLASS(TestObj);
XLUA_DECLARE_CLASS(Derived);
