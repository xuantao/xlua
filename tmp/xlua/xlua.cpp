#include "xlua_support.h"

int main(int argc, char* argv[]) {
    xlua::State s;
    xlua::Support<int>::Check<int>(&s, 1);
    //xlua::Support<int>::Check<int&>(&s, 1);

    //xlua::Support<char*>::Check<const char*>(&s, 1);
    xlua::Support<char*>::Check<const char*>(&s, 1);

    xlua::Support<std::string>::Check<std::string>(&s, 1);

    return 0;
}
