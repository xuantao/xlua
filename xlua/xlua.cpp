#include "xlua.h"
#include "detail/global.h"

XLUA_NAMESPACE_BEGIN

bool Startup() {
    return detail::GlobalVar::Startup();
}

void Shutdown() {
    auto global = detail::GlobalVar::GetInstance();
    if (global == nullptr)
        return;

    global->Purge();
}

xLuaState* Create(const char* export_module) {
    auto global = detail::GlobalVar::GetInstance();
    if (global == nullptr)
        return nullptr;

    return global->Create(export_module);
}

xLuaState* Attach(lua_State* l, const char* export_module) {
    auto global = detail::GlobalVar::GetInstance();
    if (global == nullptr)
        return nullptr;

    return global->Attach(l, export_module);
}

XLUA_NAMESPACE_END
