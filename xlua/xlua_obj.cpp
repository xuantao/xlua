#include "xlua_obj.h"
#include "xlua.h"

XLUA_NAMESPACE_BEGIN

void xLuaObjBase::AddRef() {
    if (ary_index_ != -1)
        lua_->AddObjRef(ary_index_);
}

void xLuaObjBase::UnRef() {
    if (ary_index_ != -1) {
        lua_->UnRefObj(ary_index_);
        ary_index_ = -1;
    }
}

XLUA_NAMESPACE_END
