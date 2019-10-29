#include "xlua.h"
#include "export.h"

XLUA_NAMESPACE_BEGIN

#define _XLUA_ALIGN_SIZE(S) (S + (sizeof(void*) - S % sizeof(void*)) % sizeof(void*))

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
        void* ptr = nullptr;
        const TypeDesc* desc = nullptr;
        //if (lud.weak_index <= XLUA_MAX_WEAKOBJ_TYPE_COUNT) {
        //    auto cache = GetWeakObjData(lud.weak_index, lud.ref_index);
        //    if (cache.desc->weak_proc.getter(lud.ToWeakRef()) != nullptr) {
        //        ptr = cache.obj;
        //        desc = cache.desc;
        //    }
        //} else if (lud.lud_index > XLUA_MAX_WEAKOBJ_TYPE_COUNT){
        //    ptr = lud.ToObj();
        //    desc = GetLudTypeDesc(lud.lud_index);
        //}

        lua_pushnumber(l, desc ? desc->id : 0);
        lua_pushlightuserdata(l, ptr);
        lua_pushlightuserdata(l, const_cast<TypeDesc*>(desc));
        return 3;
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
    class SerialAllocator {
        struct AllocNode {
            AllocNode* next;
            size_t capacity;
        };
    public:
        SerialAllocator(size_t block)
            : block_size_(block)
            , alloc_node_(nullptr) {
        }

        ~SerialAllocator() {
            auto* node = alloc_node_;
            alloc_node_ = nullptr;
            while (node) {
                auto* tmp = node;
                node = node->next;

                delete[] reinterpret_cast<int8_t*>(node);
            }
        }

        SerialAllocator(const SerialAllocator&) = delete;
        SerialAllocator& operator = (const SerialAllocator&) = delete;

    public:
        void* Alloc(size_t s) {
            AllocNode* node = alloc_node_;
            s = _XLUA_ALIGN_SIZE(s);

            while (node && node->capacity < s)
                node = node->next;

            if (node == nullptr) {
                size_t ns = s + kNodeSize;
                ns = block_size_ * (1 + ns / block_size_);

                node = reinterpret_cast<AllocNode*>(new int8_t[ns]);
                node->next = alloc_node_;
                node->capacity = ns - kNodeSize;
            }

            node->capacity -= s;
            return reinterpret_cast<int8_t*>(node) + kNodeSize + node->capacity;
        }

        template <typename Ty, typename... Args>
        inline Ty* AllocObj(Args&&... args) {
            return new (Alloc(sizeof(Ty)))(std::forward<Args>(args)...);
        }

    private:
        static constexpr size_t kNodeSize = _XLUA_ALIGN_SIZE(sizeof(AllocNode));

    private:
        size_t block_size_;
        AllocNode* alloc_node_;
    };

    struct ArrayObj {
        union {
            void* ptr;  // cache object ptr
            int next;   // when slot is empty, next empty slot position
        };
        int serial;
    };

    struct LudType {
        enum class Type {
            kNone,
            kPtr,
            kWeakObj,
        };

        Type type = Type::kNone;
        union {
            const TypeDesc* desc;
            size_t tag;
        };
    };

    static constexpr int kAryObjInc = 4096;
    static constexpr int kMaxLudIndex = 0xff;

    static struct Env {
        struct {
            std::array<LudType, 256> lud_list;
            std::vector<const TypeDesc*> desc_list{nullptr};
            // for light userdata weak obj refernce data cache
            std::array<std::vector<WeakObjCache>, 256> weak_data_list;
        } declared;

        struct {
            int serial_gener = 0;
            int empty_slot = 0;
            std::vector<ArrayObj> objs{ArrayObj()};
        } obj_ary;

        ExportNode* node_head = nullptr;
        SerialAllocator allocator{8*1024};
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
    WeakObjRef MakeWeakObjRef(void* ptr, ObjectIndex& index) {
        auto& ary = g_env.obj_ary;
        // alloc obj slots
        if (ary.empty_slot == 0) {
            size_t sz = ary.objs.size();
            size_t ns = sz + kAryObjInc;
            ary.objs.resize(ns);
            for (size_t i = ns; i > sz; --i) {
                auto& slot = ary.objs[i-1];
                slot.serial = 0;
                slot.next = (int)i;
            }
            ary.objs[ns - 1].next = 0;
            ary.empty_slot = (int)sz;
        }

        int idx = ary.empty_slot;
        auto& obj = ary.objs[idx];
        ary.empty_slot = obj.next;
        index.index_ = idx;

        obj.ptr = ptr;
        obj.serial = ++ary.serial_gener;
        return WeakObjRef{idx, obj.serial};
    }

    void* GetWeakObjPtr(WeakObjRef ref) {
        auto& ary = g_env.obj_ary;
        if (ref.index <= 0 && ary.objs.size() <= ref.index)
            return nullptr;

        auto& obj = ary.objs[ref.index];
        return obj.serial == ref.serial ? obj.ptr : nullptr;
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    const TypeDesc* GetLudTypeDesc(int lud_index) {
        //auto& declared = g_env.declared;
        //if (lud_index <= XLUA_MAX_WEAKOBJ_TYPE_COUNT || lud_index >= declared.lud_index_gener)
        //    return nullptr;
        //return declared.lud_list[lud_index];
        return nullptr;
    }

    WeakObjCache GetWeakObjData(int weak_index, int obj_index) {
        auto& caches = g_env.declared.weak_data_list[weak_index];
        if (caches.size() > obj_index)
            return caches[obj_index];
        return WeakObjCache{0, 0};
    }

    void SetWeakObjData(int weak_idnex, int obj_index, void* obj, const TypeDesc* desc) {
        assert(weak_idnex > 0 && weak_idnex <= kMaxLudIndex);
        if (weak_idnex <= 0)
            return;

        auto& caches = g_env.declared.weak_data_list[weak_idnex];
        if (obj_index >= caches.size)
            caches.resize((obj_index / kAryObjInc + 1) * kAryObjInc, WeakObjCache{nullptr, nullptr});

        auto& c = caches[obj_index];
        if (c.desc == nullptr || IsBaseOf(c.desc, desc))
            c = WeakObjCache{obj, desc};
    }

    bool CheckLightData(LightData ld, const TypeDesc* desc) {
        const auto& info = g_env.declared.lud_list[ld.lud_index];
        if (info.type == LudType::Type::kWeakObj) {
            auto data = GetWeakObjData(ld.weak_index, ld.ref_index);
            return data.desc != nullptr && IsBaseOf(desc, data.desc);
        } else if (info.type == LudType::Type::kPtr) {
            return info.desc != nullptr && IsBaseOf(desc, info.desc);
        }
        return false;
    }

    void* UnpackLightData(LightData ld, const TypeDesc* desc) {
        const auto& info = g_env.declared.lud_list[ld.lud_index];
        if (info.type == LudType::Type::kWeakObj) {
            auto data = GetWeakObjData(ld.weak_index, ld.ref_index);
            if (!IsBaseOf(desc, data.desc))
                return nullptr;
            if (data.desc->weak_proc.getter(ld.ToWeakRef()) == nullptr)
                return nullptr;
            return _XLUA_TO_SUPER_PTR(data.obj, data.desc, desc);
        } else if (info.type == LudType::Type::kPtr) {
            if (IsBaseOf(desc, info.desc))
                return nullptr;
            return _XLUA_TO_SUPER_PTR(ld.ToObj(), info.desc, desc);
        }
        return nullptr;
    }

    LightData PackLightPtr(void* obj, const TypeDesc* desc) {
        if (desc->lud_index == 0)
            return LightData::Make(nullptr);

        if (desc->weak_proc.tag) {
            WeakObjRef ref = desc->weak_proc.maker(obj);
            if (ref.index <= kMaxLudIndex) {
                SetWeakObjData(ref.index, ref.serial, obj, desc);
                return LightData::Make(desc->lud_index, ref.index, ref.serial);
            }
        } else {
            if (LightData::IsValid(obj))
                return LightData::Make(desc->lud_index, obj);
        }
        return LightData::Make(nullptr);
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    static bool InitSate(State* l) {
        return true;
    }

    static bool RegConst(State* l, const ConstValueNode* constValue) {
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
                RegConst(l, static_cast<ConstValueNode*>(node));
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
    if (index.index_ <= 0)
        return;

    auto& ary = internal::g_env.obj_ary;
    assert(index.index_ < (int)ary.objs.size());
    if (index.index_ >= (int)ary.objs.size())
        return;

    auto& ob = ary.objs[index.index_];
    ob.serial = 0;
    ob.next = ary.empty_slot;
    ary.empty_slot = index.index_;

    index.index_ = 0;
}

namespace internal {
    struct TypeCreator : public ITypeCreator
    {
        TypeCreator(const char* name, bool global, const TypeDesc* super) :
            type_name(PurifyTypeName(name)), is_global(global), super(super) {}
        virtual ~TypeCreator() { }

        void SetCaster(ITypeCaster* cast) override {
            caster = cast;
        }
        void SetWeakProc(WeakObjProc proc) override {
            weak_proc = proc;
        }
        void AddMember(const char* name, LuaFunction func, bool global) override {
            ((is_global || global) ? global_funcs : funcs).push_back(ExportFunc{PerifyMemberName(name), func});
        }
        void AddMember(const char* name, LuaIndexer getter, LuaIndexer setter, bool global) override {
            ((is_global || global) ? global_vars : vars).push_back(ExportVar{PerifyMemberName(name), getter, setter});
        }
        bool CheckRename(const char* name, bool global) const {
            return true;
        }
        const TypeDesc* Finalize() override {
            TypeDesc* desc = g_env.allocator.AllocObj<TypeDesc>();

            desc->lud_index = GetLudIndex();
            desc->weak_proc = weak_proc;
            desc->super = const_cast<TypeDesc*>(super);
            desc->caster = caster;


            vars.push_back(ExportVar());
            global_vars.push_back(ExportVar());

            funcs.push_back(ExportFunc());
            global_funcs.push_back(ExportFunc());

            return nullptr;
        }

        static std::string PurifyTypeName(const char* name) {
            while (name[0] == ':')
                ++name;

            char buf[1024];
            int idx = 0;
            while (*name) {
                if (name[0] == ':' && name[1] == ':') {
                    name += 2;
                    buf[idx++] = '.';
                } else {
                    buf[idx++] = *name;
                    ++name;
                }
            }
            buf[idx++] = 0;
            return std::string(buf, idx);
        }

        static StringView PerifyMemberName(const char* name) {
            // find the last scope
            while (const char* sub = ::strstr(name, "::"))
                name = sub + 2;
            // remove prefix: &
            if (name[0] == '&')
                ++name;
            // remove prefix: "m_"
            if (name[0] == 'm' && name[1] == '_')
                name += 2;
            // remove prefix: "lua"
            if ((name[0] == 'l' || name[0] == 'L') &&
                (name[1] == 'u' || name[1] == 'U') &&
                (name[2] == 'a' || name[2] == 'A')) {
                name += 3;
            }
            // remove prefix: '_'
            while (name[0] && name[0] == '_')
                ++name;
            // remove surfix: '_'
            size_t len = ::strlen(name);
            while (len && name[len-1]=='_')
                --len;

            return StringView{name, len};
        }

        uint8_t GetLudIndex() const {
            if (is_global)
                return 0;

            if (weak_proc.tag) {
                for (int i = 1; i <= kMaxLudIndex; ++i) {
                    auto& info = g_env.declared.lud_list[i];
                    if (info.type == LudType::Type::kNone)
                    {
                        info.type = LudType::Type::kWeakObj;
                        info.tag = weak_proc.tag;
                        return (uint8_t)i;
                    } else if (info.type == LudType::Type::kWeakObj && info.tag == weak_proc.tag) {
                        return (uint8_t)i;
                    }
                }
            } else {
                for (int i = 1; i <= kMaxLudIndex; ++i) {
                    auto& info = g_env.declared.lud_list[i];
                    if (info.type == LudType::Type::kNone) {
                        info.type = LudType::Type::kPtr;
                        return (uint8_t)i;
                    }
                }
            }
            return 0;
        }

        std::string type_name;
        bool is_global;
        const TypeDesc* super = nullptr;
        ITypeCaster* caster = nullptr;
        WeakObjProc weak_proc{0, nullptr, nullptr};
        std::vector<ExportVar> vars;
        std::vector<ExportVar> global_vars;
        std::vector<ExportFunc> funcs;
        std::vector<ExportFunc> global_funcs;
    };
}
XLUA_NAMESPACE_END
