#include "global.h"
#include "type_desc.h"
#include "../xlua_export.h"
#include <memory>
#include <algorithm>

XLUA_NAMESPACE_BEGIN

namespace detail {
    static NodeBase* s_node_head = nullptr;
    static GlobalVar* s_global = nullptr;
}

xLuaIndex::~xLuaIndex() {
    if (index_ != -1 && detail::s_global)
        detail::s_global->FreeObjIndex(index_);
}

namespace detail {
    static bool StateCmp(const std::pair<lua_State*, xLuaState*>& l, const std::pair<lua_State*, xLuaState*>& r) {
        return l.first < r.first;
    }

    void LogError(const char* fmt, ...) {
        char buf[XLUA_MAX_BUFFER_CACHE];
        int n = snprintf(buf, XLUA_MAX_BUFFER_CACHE, "xlua_err: ");

        va_list args;
        va_start(args, fmt);
        vsnprintf(buf + n, XLUA_MAX_BUFFER_CACHE - n, fmt, args);
        va_end(args);

        xLuaLogError(buf);
    }

    NodeBase::NodeBase(NodeCategory type) : category(type) {
        next = s_node_head;
        s_node_head = this;
    }

    NodeBase::~NodeBase() {
        NodeBase* node = s_node_head;
        while (node != this && node->next != this)
            node = node->next;

        if (node == this)
            s_node_head = next;
        else
            node->next = next;
        next = nullptr;
    }

    bool GlobalVar::Startup() {
        if (s_global)
            return false;

        s_global = new GlobalVar();

        /* 初始化静态数据 */
        NodeBase* node = s_node_head;
        while (node) {
            switch (node->category) {
            case NodeCategory::kType:
                static_cast<TypeNode*>(node)->func();
                break;
            case NodeCategory::kConst:
                s_global->const_infos_.push_back(static_cast<ConstNode*>(node)->func_());
                break;
            case NodeCategory::kScript:
                s_global->scripts_.push_back(static_cast<ScriptNode*>(node)->script);
                break;
            default:
                break;
            }

            node = node->next;
        }
        return true;
    }

    GlobalVar* GlobalVar::GetInstance() {
        return s_global;
    }

    void GlobalVar::Purge() {
        delete s_global;
        s_global = nullptr;
    }

    GlobalVar::GlobalVar()
        : alloc_(4096) {
        types_.reserve(256);
        external_types_.reserve(256);
        external_types_.push_back(nullptr);
    }

    GlobalVar::~GlobalVar() {
    }

    xLuaState* GlobalVar::Create(const char* export_module) {
        lua_State* l = luaL_newstate();
        luaL_openlibs(l);

        xLuaState* xl = new xLuaState(l, false);
        if (!xl->InitEnv(export_module, const_infos_, types_, scripts_)) {
            lua_close(l);
            delete xl;
            xl = nullptr;
        } else {
            states_.push_back(xl);
        }
        return xl;
    }

    xLuaState* GlobalVar::Attach(lua_State* l, const char* export_module) {
        xLuaState* xl = new xLuaState(l, true);
        if (!xl->InitEnv(export_module, const_infos_, types_, scripts_)) {
            delete xl;
            xl = nullptr;
        } else {
            states_.push_back(xl);
        }
        return xl;
    }

    void GlobalVar::Destory(xLuaState* l) {
        auto it = std::find(states_.begin(), states_.end(), l);
        assert(it != states_.end());
        states_.erase(it);
        delete l;
    }

    ITypeDesc* GlobalVar::AllocType(TypeCategory category,
        bool is_weak_obj,
        const char* name,
        const TypeInfo* super) {
        return new TypeDesc(s_global, category, is_weak_obj , name, super);
    }

    const TypeInfo* GlobalVar::GetTypeInfo(const char* name) const {
        if (name == nullptr || *name == 0)
            return nullptr;

        for (const auto* info : types_) {
            if (0 == strcmp(name, info->type_name))
                return info;
        }
        return nullptr;
    }

    const TypeInfo* GlobalVar::GetExternalTypeInfo(int index) const {
        assert(index < (int)external_types_.size());
        if (index >= (int)external_types_.size())
            return nullptr;
        return external_types_[index];
    }

    ArrayObj* GlobalVar::AllocObjIndex(xLuaIndex& obj_index, void* obj, const TypeInfo* info) {
        if (obj_index.index_ != -1) {
            // 总是存储子类指针
            auto& ary_obj = obj_array_[obj_index.index_];
            if (!IsBaseOf(info, ary_obj.info_)) {
                ary_obj.obj_ = obj;
                ary_obj.info_ = info;
            }
            return &ary_obj;
        } else {
            // 无可用元素，增加数组容量
            if (free_index_.empty()) {
                size_t old_size = obj_array_.size();
                size_t new_size = obj_array_.size() + XLUA_CONTAINER_INCREMENTAL;
                obj_array_.resize(new_size, ArrayObj{0, nullptr, nullptr});

                if (free_index_.capacity() < XLUA_CONTAINER_INCREMENTAL)
                    free_index_.reserve(XLUA_CONTAINER_INCREMENTAL);

                for (size_t i = new_size; i > old_size; --i)
                    free_index_.push_back((int)i - 1);
            }

            auto index = free_index_.back();
            auto& ary_obj = obj_array_[index];
            free_index_.pop_back();

            ary_obj.info_ = info;
            ary_obj.obj_ = obj;
            ary_obj.serial_num_ = ++serial_num_gener_;

            obj_index.index_ = index;
            return &ary_obj;
        }
    }

    ArrayObj* GlobalVar::GetArrayObj(int index) {
        if (index < 0 || index >= (int)obj_array_.size())
            return nullptr;
        return &obj_array_[index];
    }

    void GlobalVar::FreeObjIndex(int index) {
        auto& ary_obj = obj_array_[index];
        ary_obj.info_ = nullptr;
        ary_obj.obj_ = nullptr;
        ary_obj.serial_num_ = 0;

        if ((free_index_.capacity() - free_index_.size()) == 0)
            free_index_.reserve(free_index_.size() + XLUA_CONTAINER_INCREMENTAL);
        free_index_.push_back(index);
    }

    void GlobalVar::AddTypeInfo(TypeInfo* info) {
        assert(types_.size() < 0xff);
        types_.push_back(info);

        if (info->category != TypeCategory::kGlobal)
            info->index = ++type_index_gener_;
        else
            info->index = 0;

        if (info->is_weak_obj || info->category == TypeCategory::kExternal) {
            info->external_type_index = (int8_t)external_types_.size();
            external_types_.push_back(info);
        } else {
            info->external_type_index = 0;
        }
    }
} // namespace detail

XLUA_NAMESPACE_END
