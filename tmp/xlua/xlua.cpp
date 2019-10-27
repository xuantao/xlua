#include "xlua.h"
#include "export.h"

XLUA_NAMESPACE_BEGIN

namespace script {
#include "scripts.hpp"
}

namespace meta {
    using namespace internal;
    static int __gc(lua_State* l) {
        GetState(l)->state_.OnGc(static_cast<FullData*>(lua_touserdata(l, 1)));
        return 0;
    }

    /* call alone userdata destruction */
    static int __gc_alone(lua_State* l) {
        void* p = lua_touserdata(l, 1);
        IAloneUd* ud = static_cast<IAloneUd*>(p);
        ud->~IAloneUd();
        return 0;
    }

    static int __index_global(lua_State* l) {
        auto* indexer = static_cast<LuaIndexer>(lua_touserdata(l, 1));
        return indexer(GetState(l), nullptr, nullptr);
    }

    static int __index_member(lua_State* l) {
        auto* ud = static_cast<FullData*>(lua_touserdata(l, 1));
        auto* indexer = static_cast<LuaIndexer>(lua_touserdata(l, 2));
        ASSERT_FUD(ud);
        return indexer(GetState(l), ud->obj, ud->desc);
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    // unpack lud ptr, return (id, obj ptr, type desc)
    static int __unpack_lud(lua_State* l) {
        auto lud = LightData::Make(lua_touserdata(l, 1));
        int id = 0;
        if (lud.weak_index <= GetMaxWeakIndex()) {
            //TODO:
        } else {
            //TODO:
        }
        lua_pushnumber(l, id);
        return 1;
    }

    static int __index_lud(lua_State* l) {
        auto* ptr = lua_touserdata(l, 1);
        auto* desc = static_cast<const TypeDesc*>(lua_touserdata(l, 2));
        auto* indexer = static_cast<LuaIndexer>(lua_touserdata(l, 3));
        return indexer(GetState(l), ptr, desc);
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    static int __index_collection(lua_State* l) {
        auto* ud = static_cast<FullData*>(lua_touserdata(l, 1));
        ASSERT_FUD(ud);
        return ud->collection->Index(ud->obj, GetState(l));
    }

    static int __newindex_collection(lua_State* l) {
        auto* ud = static_cast<FullData*>(lua_touserdata(l, 1));
        ASSERT_FUD(ud);
        return ud->collection->NewIndex(ud->obj, GetState(l));
    }

    static int __len_collection(lua_State* l) {
        auto* ud = static_cast<FullData*>(lua_touserdata(l, 1));
        ASSERT_FUD(ud);
        lua_pushnumber(l, ud->collection->Length(ud->obj));
        return 1;
    }

    static int __pairs_collection(lua_State* l) {
        auto* ud = static_cast<FullData*>(lua_touserdata(l, 1));
        ASSERT_FUD(ud);
        return ud->collection->Iter(ud->obj, GetState(l));
    }

    static int __tostring_collection(lua_State* l) {
        auto* ud = static_cast<FullData*>(lua_touserdata(l, 1));
        ASSERT_FUD(ud);
        char buf[256];
        snprintf(buf, 256, "%s(%p)", ud->collection->Name(), ud->obj);
        lua_pushstring(l, buf);
        return 1;
    }
}

namespace internal {
    struct ArrayObj {
        void* obj;
        int serial;
    };

    static constexpr int kAryObjInc = 4096;

    static struct {
        struct {
            std::vector<const TypeDesc*> desc_list;
            std::vector<const TypeDesc*> lud_list;
            std::vector<const TypeDesc*> weak_list {nullptr};
        } declared;

        struct {
            int serial_gener = 0;
            std::vector<int> free_list;
            std::vector<ArrayObj> obj_list;
        } obj_ary;

        ExportNode* node_head;
        std::vector<std::vector<WeakObjCache>> weak_data_list;
        std::vector<std::pair<lua_State*, State*>> state_list;
    } g_env;

    /* append export node to list tail */
    void AppendNode(ExportNode* node) {
        if (g_env.node_head == nullptr) {
            g_env.node_head = node;
        } else {
            auto* tail = g_env.node_head;
            while (tail->next)
                tail = tail->next;
            tail->next = node;
        }
    }

    /* xlua weak obj reference support */
    WeakObjRef MakeWeakObjRef(void* obj, ObjectIndex& index) {
        auto& ary = g_env.obj_ary;
        int idx = 0;
        if (ary.free_list.empty()) {
            idx = (int)ary.obj_list.size();
            ary.obj_list.resize(kAryObjInc + idx);
            ary.free_list.reserve(kAryObjInc - idx);

            for (int i = 0; i < kAryObjInc; ++i)
                ary.free_list.push_back(idx + kAryObjInc - i);
        } else {
            idx = ary.free_list.back();
            ary.free_list.pop_back();
        }

        index.index_ = idx;
        auto& ary_obj = ary.obj_list[idx];
        ary_obj.obj = obj;
        ary_obj.serial = ++ary.serial_gener;
        return WeakObjRef{idx, ary_obj.serial};
    }

    void* GetWeakObjPtr(WeakObjRef ref) {
        auto& ary = g_env.obj_ary;
        if (ref.index < 0 && ary.obj_list.size() <= ref.index)
            return nullptr;

        auto& ary_obj = ary.obj_list[ref.index];
        return ary_obj.serial == ref.serial ? ary_obj.obj : nullptr;
    }

    const TypeDesc* GetTypeDesc(int lud_index) {
        auto& declared = g_env.declared;
        lud_index -= (int)declared.weak_list.size();
        if (lud_index < 0 || lud_index >= (int)declared.lud_list.size())
            return nullptr;
        return declared.lud_list[lud_index];
    }

    WeakObjCache GetWeakObjData(int weak_index, int obj_index) {
        return WeakObjCache{0, 0};
    }

    void SetWeakObjData(int weak_idnex, int obj_index, void* obj, const TypeDesc* desc) {

    }

    int GetMaxWeakIndex() {
        return 0;
    }

    static bool InitSate(State* l) {
        return true;
    }

    static bool RegConst(State* l, const internal::ConstValue* constValue) {
        return true;
    }

    static bool RegScript(State* l, const ScriptNode* script) {
        return true;
    }

    static bool RegDeclared(State* l, const TypeDesc* desc) {
        return true;
    }

    static void Reg(State* l, const char* mod) {
        auto* node = g_env.node_head;
        // reg const value and reg type
        while (node) {
            if (node->type == NodeType::kConst)
                RegConst(l, static_cast<ConstValue*>(node));
            else if (node->type == NodeType::kType)
                static_cast<TypeNode*>(node)->Reg();
            node = node->next;
        }

        // reg liternal script
        node = g_env.node_head;
        while (node) {
            if (node->type == NodeType::kScript)
                RegScript(l, static_cast<ScriptNode*>(node));
            node = node->next;
        }

        // reg declared type
        for (const auto* desc : g_env.declared.desc_list) {
            RegDeclared(l, desc);
        }
    }
} // namespace internal


State* CreateState(const char* mod) {
    return nullptr;
}

State* AttachState(lua_State* l, const char* mod) {
    return nullptr;
}

void FreeObjectIndex(ObjectIndex& index) {
    if (index.index_ < 0)
        return;

    auto& ary = internal::g_env.obj_ary;

    assert(index.index_ < (int)ary.obj_list.size());

    auto& ary_obj = ary.obj_list[index.index_];
    ary_obj.obj = nullptr;
    ary_obj.serial = 0;
    ary.free_list.push_back(index.index_);

    index.index_ = -1;
}

XLUA_NAMESPACE_END
