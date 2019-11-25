#include "lua_export.h"
#include "test.h"
#include "gtest/gtest.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
