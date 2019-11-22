#include "lua_export.h"
#include "test.h"
#include "gtest/gtest.h"
#include <stdio.h>
#include <xlua.h>

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    system("pause");
    return ret;
}
