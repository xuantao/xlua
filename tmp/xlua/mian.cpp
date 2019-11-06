#include <iostream>
#include "xlua.h"
#include "export_prepare.h"

int main(int argc, char* argv) {
    test_export();

    static_assert(xlua::IsCollctionType<std::vector<int>>::value, "");
    static_assert(xlua::IsCollctionType<std::vector<std::vector<int>>>::value, "");
    static_assert(xlua::IsSupport<std::vector<int>>::value, "");
    static_assert(xlua::IsSupport<std::vector<std::vector<int>>>::value, "");

    return 0;
}
