#include <iostream>
#include "xlua.h"
#include "export_prepare.h"

int main(int argc, char* argv) {
    test_export();

    xLuaGetCollection(xlua::Identity<std::vector<int>>());
    //static_assert(xlua::IsCollectionType<std::vector<int>>::value, "not collection?");
    //static_assert(xlua::IsCollectionType<std::vector<std::vector<int>>>::value, "");
    //static_assert(xlua::IsSupport<std::vector<int>>::value, "");
    //static_assert(xlua::IsSupport<std::vector<std::vector<int>>>::value, "");

    return 0;
}
