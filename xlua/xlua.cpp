#include "xlua.h"
#include "detail/global.h"

void xLuaPush(xlua::xLuaState* l, std::nullptr_t) {
    l->Push(nullptr);
}

XLUA_NAMESPACE_BEGIN

xLuaIndex::~xLuaIndex() {
    auto global = detail::GlobalVar::GetInstance();
    if (index_ != -1 && global)
        global->FreeObjIndex(index_);
}

void xLuaLogBuffer::Log(const char* fmt, ...) {
    if (write_ >= capacity_)
        return;

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf_ + write_, capacity_ - write_, fmt, args);
    va_end(args);
    if (n > 0) {
        write_ += n;
        if (write_ < capacity_)
            buf_[write_++] = '\n';
    }
}

bool Startup(LogFunc fn) {
    return detail::GlobalVar::Startup(fn);
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
