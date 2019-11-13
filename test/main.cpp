#include "lua_export.h"
#include "test.h"
#include "gtest/gtest.h"
#include <stdio.h>
#include <xlua.h>

TEST(first_test, test_1) {

}

int main(int argc, char* argv[]) {
//    xlua::Startup(nullptr);
    //xlua::State* l = xlua::CreateState(nullptr);

    //l->LoadGlobal("_G");
    //assert(xlua::VarType::kTable == l->GetType(-1));
    //l->PopTop(1);

    //TestPushLoad(l);
    //TestExport(l);
    //TestGlobal(l);

    //Triangle* triangle = nullptr;
    //l->Push(triangle);

    ////l->Release();
    //xlua::DestoryState(l);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
