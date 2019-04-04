#include <stdio.h>
#include <xlua.h>
#include "logic.h"
#include "lua_export.h"
#include "test.h"



const char* s = R"V0G0N(
             O freddled gruntbuggly thy micturations are to me
                 As plured gabbleblochits on a lurgid bee.
              Groop, I implore thee my foonting turlingdromes.
           And hooptiously drangle me with crinkly bindlewurdles,
Or I will rend thee in the gobberwarts with my blurlecruncheon, see if I don't.

                "(by Prostetnic Vogon Jeltz; see p. 56/57)
)V0G0N";

#define STR_TEXT(TEXT_) TEXT_

const char* const s3 = STR_TEXT(R"V0G0N(
whtf
)V0G0N");

int main(int argc, char* argvp[]) {
    xlua::Startup();
    xlua::xLuaState* l = xlua::Create(nullptr);

    //Quard* m = nullptr;
    //l->Push(m);
    //LOG_TOP_(l);
    //Color c1{1, 2, 3, 4};
    //l->Push(c1);
    //LOG_TOP_(l);
    //auto c2 = l->Load<Color>(-1);
    //LOG_TOP_(l);

    //Vec2 v1{1.0f, 2.0f};
    //l->Push(v1);
    //Vec2 v2 = l->Load<Vec2>(-1);

    //Quard qm;
    //l->Push(qm);

    //Quard gm1 = l->Load<Quard>(-1);

    //lua_settop(l->GetState(), 0);

    //lua_gc(l->GetState(), LUA_GCCOLLECT, 0);

    //printf("%s\n", s);
    //printf("111\n%s\n222\n", s3);

    TestGlobal(l);

    l->Release();
    return 0;
}
