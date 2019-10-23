#include "support.h"
#include "xlua.h"

XLUA_NAMESPACE_BEGIN

namespace internal {
    /* xlua weak obj reference support */
    WeakObjRef MakeWeakObjRef(void* obj, ObjectIndex& index) {
        //TODO:
        return WeakObjRef{0, 0};
    }
    void* GetWeakObjPtr(WeakObjRef ref) {
        //TODO:
        return nullptr;
    }

    WeakObjData GetWeakObjData(int weak_index, int obj_index) {
        return WeakObjData{0, 0};
    }

    void SetWeakObjData(int weak_idnex, int obj_index, void* obj, const TypeDesc* desc) {

    }

}

void FreeObjectIndex(ObjectIndex& index) {
    //TODO:
    index.index_ = 0;
}

XLUA_NAMESPACE_END
