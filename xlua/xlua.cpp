#include "xlua.h"
#include "xlua_state.h"
#include "xlua_export.h"

XLUA_NAMESPACE_BEGIN

#define _XLUA_ALIGN_SIZE(S) (S + (sizeof(void*) - S % sizeof(void*)) % sizeof(void*))

static constexpr size_t kBuffCacheSize = 1024;
static constexpr size_t kMaxLudObjIndex = 0x00ffffff;

namespace script {
#include "scripts.hpp"
}

namespace internal {
    class SerialAlloc {
        struct AllocNode {
            AllocNode* next;
            size_t capacity;
        };
    public:
        SerialAlloc(size_t block)
            : block_size_(block)
            , alloc_node_(nullptr) {
        }

        ~SerialAlloc() {
            auto* node = alloc_node_;
            alloc_node_ = nullptr;
            while (node) {
                auto* tmp = node;
                node = node->next;

                delete[] reinterpret_cast<int8_t*>(node);
            }
        }

        SerialAlloc(const SerialAlloc&) = delete;
        SerialAlloc& operator = (const SerialAlloc&) = delete;

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
            return new (Alloc(sizeof(Ty))) Ty(std::forward<Args>(args)...);
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

    struct ExportVar {
        const char* name;
        LuaIndexer getter;
        LuaIndexer setter;
    };

    struct ExportFunc {
        const char* name;
        LuaFunction func;
    };

    struct TypeVar {
        size_t len;
        ExportVar* data;
    };

    struct TypeFunc {
        size_t len;
        ExportFunc* data;
    };

    struct TypeData : TypeDesc {
        TypeVar member_vars;
        TypeVar global_vars;
        TypeFunc member_funcs;
        TypeFunc global_funcs;
    };

    static constexpr int kAryObjInc = 4096;
    static constexpr int kMaxLudIndex = 0xff;

    /* xlua env data */
    struct Env{
        Env() = default;
        Env(const Env&) = delete;
        void operator = (const Env&) = delete;

        struct {
            std::array<LudType, 256> lud_list;
            std::vector<TypeData*> desc_list{nullptr};
            // for light userdata weak obj refernce data cache
            std::vector<size_t> weak_tags{0};
            std::array<std::vector<const TypeDesc*>, 256> weak_data_list;
        } declared;

        struct {
            int serial_gener = 0;
            int empty_slot = 0;
            std::vector<ArrayObj> objs{ArrayObj()};
        } obj_ary;

        SerialAlloc allocator{8*1024};
        std::vector<std::pair<lua_State*, State*>> state_list;
    };

    /* seperate the global export node list */
    static ExportNode* g_node_head = nullptr;
    static Env g_env;

    State* GetState(lua_State* l) {
        auto it = std::find_if(g_env.state_list.begin(), g_env.state_list.end(),
            [l](const std::pair<lua_State*, State*>& pair) {
            return pair.first == l;
        });
        return it == g_env.state_list.end() ? nullptr : it->second;
    }

    void Destory(State* s) {
        //TODO: how to detach state
        if (!s->state_.is_attach_)
            lua_close(s->state_.l_);
        delete s;
    }

    const TypeDesc* GetTypeDesc(const char* name) {
        for (auto* data : g_env.declared.desc_list) {
            if (strcmp(data->name, name) == 0)
                return data;
        }
        return nullptr;
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    static const TypeDesc* GetWeakObjDesc(int weak_index, int obj_index) {
        auto& caches = g_env.declared.weak_data_list[weak_index];
        if (caches.size() > obj_index)
            return caches[obj_index];
        return nullptr;
    }

    static void SetWeakObjDesc(int weak_idnex, int obj_index, const TypeDesc* desc) {
        assert(weak_idnex > 0 && weak_idnex <= kMaxLudIndex);
        if (weak_idnex <= 0)
            return;

        auto& caches = g_env.declared.weak_data_list[weak_idnex];
        if (obj_index >= (int)caches.size())
            caches.resize((obj_index / kAryObjInc + 1) * kAryObjInc, nullptr);

        auto d = caches[obj_index];
        if (d == nullptr || IsBaseOf(d, desc))
            caches[obj_index] = desc;
    }

    const TypeDesc* GetLightUdDesc(LightUd ld) {
        const auto& info = g_env.declared.lud_list[ld.lud_index];
        if (info.type == LudType::Type::kWeakObj) {
            return GetWeakObjDesc(ld.weak_index, ld.ref_index);
        } else if (info.type == LudType::Type::kPtr) {
            return info.desc;
        }
        return nullptr;
    }

    bool CheckLightUd(LightUd ld, const TypeDesc* desc) {
        auto* cache_desc = GetLightUdDesc(ld);
        return (cache_desc && IsBaseOf(desc, cache_desc));
    }

    void* UnpackLightUd(LightUd ld, const TypeDesc* desc) {
        const auto& info = g_env.declared.lud_list[ld.lud_index];
        if (info.type == LudType::Type::kWeakObj) {
            auto* cache_desc = GetWeakObjDesc(ld.weak_index, ld.ref_index);
            if (!IsBaseOf(desc, cache_desc))
                return nullptr;

            void* ptr = cache_desc->weak_proc.getter(ld.ToWeakRef());
            if (ptr == nullptr)
                return nullptr;
            return _XLUA_TO_SUPER_PTR(ptr, cache_desc, desc);
        } else if (info.type == LudType::Type::kPtr) {
            if (!IsBaseOf(desc, info.desc))
                return nullptr;
            return _XLUA_TO_SUPER_PTR(ld.ToObj(), info.desc, desc);
        }
        return nullptr;
    }

    LightUd PackLightUd(void* obj, const TypeDesc* desc) {
        if (desc->lud_index == 0)
            return LightUd::Make(nullptr);

        if (desc->weak_index) {
            WeakObjRef ref = desc->weak_proc.maker(obj);
            if (ref.index <= kMaxLudObjIndex) {
                SetWeakObjDesc(ref.index, ref.serial, desc);
                return LightUd::Make(desc->lud_index, ref.index, ref.serial);
            }
        } else {
            if (!LightUd::IsValid(obj))
                return LightUd::Make(desc->lud_index, obj);
        }
        return LightUd::Make(nullptr);
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
}

namespace meta {
    using namespace internal;
    static int __gc(lua_State* l) {
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 1));
        assert(ud && ud->IsValid());
        GetState(l)->state_.OnGc(ud);
        return 0;
    }

    /* call alone userdata destruction */
    static int __gc_alone(lua_State* l) {
        void* p = lua_touserdata(l, 1);
        auto* o = static_cast<IObjData*>(p);
        o->~IObjData();
        return 0;
    }

    static int __index_global(lua_State* l) {
        /* the bottom stack value is __newindex value */
        auto* indexer = (LuaIndexer)(lua_touserdata(l, 2));
        auto* state = (State*)(lua_touserdata(l, 3));
        auto* desc = (const TypeDesc*)(lua_touserdata(l, 4));
        return indexer(state, nullptr, desc);
    }

    static int __index_member(lua_State* l) {
        /* the bottom stack value is __newindex value */
        auto* indexer = static_cast<LuaIndexer>(lua_touserdata(l, 2));
        auto* state = static_cast<State*>(lua_touserdata(l, 3));
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 4));

        if (!ud->IsValid()) {
            //TODO: error
            assert(false);
            return 0;
        }

        auto* obj = ud->ptr;
        if (ud->major == UdMajor::kDeclaredType && ud->minor == UdMinor::kPtr && ud->desc->weak_index)
            obj = ud->desc->weak_proc.getter(ud->ref);
        if (obj == nullptr) {
            //TODO: error
            assert(false);
            return 0;
        }

        return indexer(state, ud->ptr, ud->desc);
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    // unpack lud ptr, return (id, obj_ptr, type_desc)
    static int __unpack_lud(lua_State* l) {
        auto lud = LightUd::Make(lua_touserdata(l, 1));
        void* ptr = nullptr;
        const TypeDesc* desc = nullptr;
        auto& lty = g_env.declared.lud_list[lud.lud_index];
        if (lty.type == LudType::Type::kWeakObj) {
            desc = GetWeakObjDesc(lud.lud_index, lud.ref_index);
            ptr = desc->weak_proc.getter(lud.ToWeakRef());
            if (ptr == nullptr)
                desc = nullptr;
        } else if (lty.type == LudType::Type::kPtr) {
            ptr = lud.ToObj();
            desc = lty.desc;
        }

        lua_pushnumber(l, desc ? desc->id : 0);
        lua_pushlightuserdata(l, ptr);
        lua_pushlightuserdata(l, const_cast<TypeDesc*>(desc));
        return 3;
    }

    static int __index_lud(lua_State* l) {
        /* the bottom stack value is __newindex value */
        auto* indexer = (LuaIndexer)(lua_touserdata(l, 2));     // indexer function
        auto* state = (State*)(lua_touserdata(l, 3));           // xlua state
        auto* ptr = lua_touserdata(l, 4);                       // obj
        auto* desc = (const TypeDesc*)(lua_touserdata(l, 5));   // type desc
        lua_settop(l, 1);                                       // pop all not uesed any alue
        return indexer(state, ptr, desc);
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    static int __index_collection(lua_State* l) {
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 1));
        assert(ud && ud->IsValid() && ud->major == internal::UdMajor::kCollection);
        return ud->collection->Index(ud->ptr, GetState(l));
    }

    static int __newindex_collection(lua_State* l) {
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 1));
        assert(ud && ud->IsValid() && ud->major == internal::UdMajor::kCollection);
        return ud->collection->NewIndex(ud->ptr, GetState(l));
    }

    static int __len_collection(lua_State* l) {
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 1));
        assert(ud && ud->IsValid() && ud->major == internal::UdMajor::kCollection);
        lua_pushnumber(l, ud->collection->Length(ud->ptr));
        return 1;
    }

    static int __pairs_collection(lua_State* l) {
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 1));
        assert(ud && ud->IsValid() && ud->major == internal::UdMajor::kCollection);
        return ud->collection->Iter(ud->ptr, GetState(l));
    }

    static int __tostring_collection(lua_State* l) {
        auto* ud = static_cast<FullUd*>(lua_touserdata(l, 1));
        assert(ud && ud->IsValid() && ud->major == internal::UdMajor::kCollection);
        char buf[256];
        snprintf(buf, 256, "%s(%p)", ud->collection->Name(), ud->ptr);
        lua_pushstring(l, buf);
        return 1;
    }
} // namespace meta

namespace utility {
    struct UdInfo {
        internal::UdMajor major = internal::UdMajor::kNone;
        internal::UdMinor minor = internal::UdMinor::kNone;
        void* ud = nullptr;

        union {
            const TypeDesc* desc;
            ICollection* collection;
        };
        union {
            void* obj;
            WeakObjRef ref;
        };

        explicit inline operator bool() const {
            return major != internal::UdMajor::kNone &&
                minor != internal::UdMinor::kNone &&
                ud != nullptr;
        }
    };

    static UdInfo GetUdInfo(lua_State* l, int index) {
        UdInfo info;
        int l_ty = lua_type(l, index);
        if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            auto ud = internal::LightUd::Make(lua_touserdata(l, index));
            const TypeDesc* desc = ud ? GetLightUdDesc(ud) : nullptr;
            if (desc) {
                info.major = internal::UdMajor::kDeclaredType;
                info.minor = internal::UdMinor::kPtr;
                info.desc = desc;
                info.ud = ud.value;
                if (desc->weak_index)
                    info.ref = ud.ToWeakRef();
                else
                    info.obj = ud.ToObj();
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
        } else if (l_ty == LUA_TUSERDATA) {
            auto* ud = static_cast<internal::FullUd*>(lua_touserdata(l, index));
            if (ud && ud->IsValid()) {
                info.major = ud->major;
                info.minor = ud->minor;
                info.ud = ud;
                info.desc = ud->desc;
                info.ref = ud->ref;
            }
        }
        return info;
    }

    static int __type(lua_State* l) {
        int l_ty = lua_type(l, 1);
        if (l_ty == LUA_TLIGHTUSERDATA || l_ty == LUA_TUSERDATA) {
            auto info = GetUdInfo(l, 1);
            if (info) {
                if (info.major == internal::UdMajor::kDeclaredType)
                    lua_pushstring(l, info.desc->name);
                else
                    lua_pushstring(l, info.collection->Name());
            } else {
                lua_pushstring(l, "unknown");
            }
        } else {
            lua_pushstring(l, lua_typename(l, 1));
        }
        return 1;
    }

    static int __is_valid(lua_State* l) {
        auto info = GetUdInfo(l, 1);
        if (!info) {
            lua_pushboolean(l, false);
        } else if (info.major == internal::UdMajor::kCollection) {
            lua_pushboolean(l, true);
        } else if (info.minor == internal::UdMinor::kPtr) {
            if (info.desc->weak_index)
                lua_pushboolean(l, info.desc->weak_proc.getter(info.ref) != nullptr);
            else
                lua_pushboolean(l, true);
        } else {
            lua_pushboolean(l, true);
        }
        return 1;
    }

    /* delcared type cast */
    static int __cast(lua_State* l) {
        auto info = GetUdInfo(l, 1);
        auto* name = lua_tostring(l, 2);
        if (!info || name == nullptr || *name == 0)
            return 0;   // invalid parameters
        if (info.major != internal::UdMajor::kDeclaredType || info.minor == internal::UdMinor::kValue)
            return 0;   // collection or value data not support cast

        const auto* desc = internal::GetTypeDesc(name);
        if (desc == nullptr)
            return 0;

        if (internal::IsBaseOf(desc, info.desc)) {
            lua_pushvalue(l, 1);
            return 1;
        }

        void* derived = nullptr;
        if (info.desc->weak_index)
            derived = ToDerived(info.desc->weak_proc.getter(info.ref), info.desc, desc);
        else
            derived = ToDerived(info.obj, info.desc, desc);
        if (derived == nullptr)
            return 0;   // dynamic_cast failed

#if XLUA_ENABLE_LUD_OPTIMIZE
        if (info.minor == internal::UdMinor::kPtr && info.desc->lud_index) {
            internal::LightUd lud;
            if (info.desc->weak_index) {
                auto ref = desc->weak_proc.maker(derived);
                lud = internal::LightUd::Make(desc->weak_index, ref.index, ref.serial);
            } else {
                lud = internal::LightUd::Make(desc->lud_index, derived);
            }

            lua_pushlightuserdata(l, lud.value);
            return 1;
        }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

        auto* ud = static_cast<internal::FullUd*>(info.ud);
        ud->desc = desc;
        if (info.desc->weak_index)
            ud->ref = desc->weak_proc.maker(derived);
        else
            ud->ptr = derived;

        lua_pushvalue(l, 1);
        return 1;
    }

    /* collection operate, insert element */
    static int __insert(lua_State* l) {
        auto info = GetUdInfo(l, 1);
        if (!info || info.major != internal::UdMajor::kCollection) {
            luaL_error(l, "insert only could process collection object");
            return 0;
        }

        return info.collection->Index(info.obj, internal::GetState(l));
    }

    /* collection operate, remove element */
    static int __remove(lua_State* l) {
        auto info = GetUdInfo(l, 1);
        if (!info || info.major != internal::UdMajor::kCollection) {
            luaL_error(l, "remove only could process collection object");
            return 0;
        }

        return info.collection->Remove(info.obj, internal::GetState(l));
    }

    /* collection operate, clear all */
    static int __clear(lua_State* l) {
        auto info = GetUdInfo(l, 1);
        if (!info || info.major != internal::UdMajor::kCollection)
            luaL_error(l, "clear only could process collection object");
        else
            info.collection->Clear(info.obj);
        return 0;
    }
}

namespace internal {
    static size_t GetVarNum(const TypeData& td, bool global) {
        size_t n = (global ? td.global_vars : td.member_vars).len;
        if (td.super)
            return n + GetVarNum(*static_cast<const TypeData*>(td.super), global);
        return n;
    }

    static size_t GetFuncNum(const TypeData& td, bool global) {
        size_t n = (global ? td.global_funcs : td.member_funcs).len;
        if (td.super)
            return n + GetFuncNum(*static_cast<const TypeData*>(td.super), global);
        return n;
    }

    /* append export node to list tail */
    void AppendNode(ExportNode* node) {
        if (g_node_head == nullptr) {
            g_node_head = node;
        } else {
            auto* tail = g_node_head;
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

    static size_t PurifyTypeName(char* buf, size_t sz, const char* name) {
        while (name[0] == ':')
            ++name;

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
        return idx;
    }

    static void MakeGlobal(State* s, const char* path) {
        char buf[1024];
        /* process export to _G table case */
        if (s->state_.module_ && *s->state_.module_) {
            if (Is_G(path))
                snprintf(buf, 1024, "%s", s->state_.module_);
            else
                snprintf(buf, 1024, "%s.%s", s->state_.module_, path);
        } else {
            strcpy_s(buf, 1024, path);
        }

        if (s->state_.LoadGlobal(buf) != LUA_TTABLE) {
            s->PopTop(1);                           // pop load value
            s->state_.NewTable();                   // create table
            lua_pushvalue(s->GetLuaState(), -1);    // copy table
            s->state_.SetGlobal(buf, true, true);   // set global & pop top
        }

        assert(s->GetTop() == 1);
    }

    static bool InitState(State* s) {
        // type desc table
        lua_createtable(s->GetLuaState(), 0, 0);
        s->state_.desc_ref_ = luaL_ref(s->GetLuaState(), LUA_REGISTRYINDEX);

        // type metatable list
        lua_createtable(s->GetLuaState(), 0, 0);
        s->state_.meta_ref_ = luaL_ref(s->GetLuaState(), LUA_REGISTRYINDEX);

        // collection metatable ref
        lua_createtable(s->GetLuaState(), 0, 5);
        lua_pushcfunction(s->GetLuaState(), &meta::__index_collection);
        lua_setfield(s->GetLuaState(), -2, "__index");

        lua_pushcfunction(s->GetLuaState(), &meta::__newindex_collection);
        lua_setfield(s->GetLuaState(), -2, "__newindex");

        lua_pushcfunction(s->GetLuaState(), &meta::__pairs_collection);
        lua_setfield(s->GetLuaState(), -2, "__pairs");

        lua_pushcfunction(s->GetLuaState(), &meta::__pairs_collection);
        lua_setfield(s->GetLuaState(), -2, "__ipairs");

        lua_pushcfunction(s->GetLuaState(), &meta::__len_collection);
        lua_setfield(s->GetLuaState(), -2, "__len");

        lua_pushcfunction(s->GetLuaState(), &meta::__tostring_collection);
        lua_setfield(s->GetLuaState(), -2, "__tostring");

        lua_pushcfunction(s->GetLuaState(), &meta::__gc);
        lua_setfield(s->GetLuaState(), -2, "__gc");
        s->state_.collection_meta_ref_ = luaL_ref(s->GetLuaState(), LUA_REGISTRYINDEX);

        // alone object metatble
        lua_createtable(s->GetLuaState(), 0, 1);
        lua_pushcfunction(s->GetLuaState(), &meta::__gc_alone);
        lua_setfield(s->GetLuaState(), -2, "__gc");
        s->state_.alone_meta_ref_ = luaL_ref(s->GetLuaState(), LUA_REGISTRYINDEX);

        // obj cache table
        lua_createtable(s->GetLuaState(), 0, 0);
        s->state_.obj_ref_ = luaL_ref(s->GetLuaState(), LUA_REGISTRYINDEX);

        // lua object cache table
        lua_createtable(s->GetLuaState(), 0, 0);
        lua_createtable(s->GetLuaState(), 0, 1);
        lua_pushstring(s->GetLuaState(), "v");
        lua_setfield(s->GetLuaState(), -2, "__mode");
        lua_setmetatable(s->GetLuaState(), -2);
        s->state_.cache_ref_ = luaL_ref(s->GetLuaState(), LUA_REGISTRYINDEX);

#if XLUA_ENABLE_LUD_OPTIMIZE
        // set light userdata metatable
        lua_pushlightuserdata(s->GetLuaState(), nullptr);                   // push nil lightuserdata
        luaL_loadbuffer(s->GetLuaState(), script::kLudMetatable,
            ::strlen(script::kLudMetatable), "lightuserdata_metatable");    // load metatable function
        lua_pushlightuserdata(s->GetLuaState(), s);                         // push state
        lua_pushcfunction(s->GetLuaState(), &meta::__unpack_lud);           // push unpack_ptr
        lua_pushcfunction(s->GetLuaState(), &meta::__index_lud);            // push indexer
        lua_geti(s->GetLuaState(), LUA_REGISTRYINDEX, s->state_.desc_ref_); // push desc_table_list
        lua_pcall(s->GetLuaState(), 4, 1, 0);                               // create metatable
        lua_setmetatable(s->GetLuaState(), -2);                             // set lightuserdata metatable
        lua_pop(s->GetLuaState(), 1);                                       // pop nil lightuserdata
#endif // XLUA_ENABLE_LUD_OPTIMIZE

        MakeGlobal(s, "xlua");
        lua_pushcfunction(s->GetLuaState(), &utility::__type);
        lua_setfield(s->GetLuaState(), -2, "Type");
        lua_pushcfunction(s->GetLuaState(), &utility::__is_valid);
        lua_setfield(s->GetLuaState(), -2, "IsValid");
        lua_pushcfunction(s->GetLuaState(), &utility::__cast);
        lua_setfield(s->GetLuaState(), -2, "Cast");
        lua_pushcfunction(s->GetLuaState(), &utility::__insert);
        lua_setfield(s->GetLuaState(), -2, "Insert");
        lua_pushcfunction(s->GetLuaState(), &utility::__remove);
        lua_setfield(s->GetLuaState(), -2, "Remove");
        lua_pushcfunction(s->GetLuaState(), &utility::__clear);
        lua_setfield(s->GetLuaState(), -2, "Clear");
        lua_pop(s->GetLuaState(), 1);

        assert(s->GetTop() == 0);
        return true;
    }

    static bool RegConst(State* s, const ConstValueNode* node) {
        char buf[1024];
        PurifyTypeName(buf, kBuffCacheSize, node->name);
        MakeGlobal(s, buf);

        const auto* val = node->gen();
        while (val->type != ConstValue::Type::kNone) {
            auto view = PurifyMemberName(val->name);
            lua_pushlstring(s->GetLuaState(), view.str, view.len);

            switch (val->type) {
            case ConstValue::Type::kFloat:
                lua_pushnumber(s->GetLuaState(), val->float_val);
                break;
            case ConstValue::Type::kInteger:
                lua_pushinteger(s->GetLuaState(), val->integer_val);
                break;
            case ConstValue::Type::kString:
                lua_pushstring(s->GetLuaState(), val->string_val);
                break;
            }

            lua_settable(s->GetLuaState(), -3);
            ++val;
        }

        // not set golbal table metatble
        if (!buf[0] || strcmp(buf, "_G") != 0) {
            //luaL_dostring(s->GetLuaState(), script::kConstMetatable);
            //lua_setmetatable(s->GetLuaState(), -2);
        }

        lua_pop(s->GetLuaState(), 1);
        assert(s->GetTop() == 0);
        return true;
    }

    static bool RegScript(State* s, const ScriptNode* node) {
        s->DoString(node->script, node->name);
        assert(s->GetTop() == 0);
        return true;
    }

    static void PushFuncs(lua_State* l, const TypeData& td, bool global) {
        if (td.super)
            PushFuncs(l, *static_cast<const TypeData*>(td.super), global);

        const auto& funcs = global ? td.global_funcs : td.member_funcs;
        for (size_t i = 0; i < funcs.len; ++i) {
            lua_pushcfunction(l, funcs.data[i].func);
            lua_setfield(l, -2, funcs.data[i].name);
        }
    }

    static void PushVars(lua_State* l, const TypeData& td, bool global) {
        if (td.super)
            PushVars(l, *static_cast<const TypeData*>(td.super), global);

        const auto& vars = global ? td.global_vars : td.member_vars;
        for (size_t i = 0; i < vars.len; ++i) {
            const auto& v = vars.data[i];
            lua_createtable(l, 2, 0);
            if (v.getter)
                lua_pushlightuserdata(l, static_cast<void*>(v.getter));
            else
                lua_pushnil(l);
            lua_seti(l, -2, 1);
            if (v.setter)
                lua_pushlightuserdata(l, static_cast<void*>(v.setter));
            else
                lua_pushnil(l);
            lua_seti(l, -2, 2);
            lua_setfield(l, -2, v.name);
        }
    }

    static bool RegDeclared(State* s, const TypeData& td) {
        // create global table
        size_t gfn = GetFuncNum(td, true);
        size_t gvn = GetVarNum(td, true);
        if (gfn || gvn) {
            MakeGlobal(s, td.name);                                 // create global table

            if (Is_G(td.name)) {
                PushFuncs(s->GetLuaState(), td, true);              // _G only accept function
            } else {
                luaL_loadbuffer(s->state_.l_, script::kGlobalMetatable,
                    ::strlen(script::kGlobalMetatable),
                    "global_metatable");                                // load meta function
                lua_pushlightuserdata(s->state_.l_, s);                 // push State*
                lua_pushlightuserdata(s->state_.l_, (TypeDesc*)&td);    // push TypeDesc*
                lua_pushcfunction(s->state_.l_, &meta::__index_global); // push indexer
                lua_pushstring(s->state_.l_, td.name);                  // push md_name
                lua_createtable(s->state_.l_, 0, (int)gfn);             // create function table
                PushFuncs(s->GetLuaState(), td, true);
                lua_createtable(s->state_.l_, 0, (int)gvn);             // create var table
                PushVars(s->GetLuaState(), td, true);
                lua_pcall(s->GetLuaState(), 6, 1, 0);                   // get metatable
                lua_setmetatable(s->GetLuaState(), -2);                 // set metatable
            }

            s->PopTop(1);                                               // pop global table
            assert(s->GetTop() == 0);
        }

        size_t fn = GetFuncNum(td, false);
        size_t vn = GetVarNum(td, false);
        // create function table
        lua_createtable(s->state_.l_, 0, (int)fn);
        PushFuncs(s->state_.l_, td, false);
        int f_index = s->GetTop();
        // create var table
        lua_createtable(s->state_.l_, 0, (int)vn);
        PushVars(s->state_.l_, td, false);
        int v_index = s->GetTop();

        lua_createtable(s->state_.l_, 0, 3);            // typedesc table
        lua_pushstring(s->state_.l_, td.name);
        lua_setfield(s->state_.l_, -2, "name");
        lua_pushvalue(s->state_.l_, -3);
        lua_setfield(s->state_.l_, -2, "funcs");
        lua_pushvalue(s->state_.l_, -2);
        lua_setfield(s->state_.l_, -2, "vars");

        lua_geti(s->state_.l_, LUA_REGISTRYINDEX, s->state_.desc_ref_);
        lua_pushvalue(s->state_.l_, -2);
        lua_seti(s->state_.l_, -2, td.id);
        lua_pop(s->state_.l_, 2);                       // desc_list_table, desc_table

        // create metatable
        luaL_loadbuffer(s->GetLuaState(), script::kFudMetatable,
            ::strlen(script::kFudMetatable), "declared_metatable");
        lua_pushlightuserdata(s->GetLuaState(), s);
        lua_pushcfunction(s->GetLuaState(), &meta::__index_member);
        lua_pushstring(s->GetLuaState(), td.name);
        lua_pushvalue(s->GetLuaState(), f_index);
        lua_pushvalue(s->GetLuaState(), v_index);
        lua_pcall(s->GetLuaState(), 5, 1, 0);
        lua_pushcfunction(s->GetLuaState(), &meta::__gc);
        lua_setfield(s->GetLuaState(), -2, "__gc");

        lua_geti(s->GetLuaState(), LUA_REGISTRYINDEX, s->state_.meta_ref_);
        lua_pushvalue(s->GetLuaState(), -2);
        lua_seti(s->GetLuaState(), -2, td.id);
        lua_pop(s->GetLuaState(), 4);   // meta_list_table, meta_table, var_table, func_table

        assert(s->GetTop() == 0);
        return true;
    }

    static void Reg(State* s) {
        auto* node = g_node_head;
        // reg const value and reg type
        while (node) {
            if (node->type == NodeType::kConst)
                RegConst(s, static_cast<ConstValueNode*>(node));
            else if (node->type == NodeType::kType)
                static_cast<TypeNode*>(node)->reg();
            node = node->next;
        }

        // reg liternal script
        node = g_node_head;
        while (node) {
            if (node->type == NodeType::kScript)
                RegScript(s, static_cast<ScriptNode*>(node));
            node = node->next;
        }

        // reg declared type
        for (size_t i = 1, c = g_env.declared.desc_list.size(); i < c; ++i) {
            RegDeclared(s, *g_env.declared.desc_list[i]);
        }
        assert(s->GetTop() == 0);
    }
} // namespace internal

State* CreateState(const char* mod) {
    lua_State* l = luaL_newstate();
    luaL_openlibs(l);

    State* s = new State();
    s->state_.l_ = l;
    s->state_.is_attach_ = false;
    s->state_.module_ = mod;

    internal::InitState(s);
    internal::Reg(s);
    internal::g_env.state_list.push_back(std::make_pair(l, s));

    assert(s->GetTop() == 0);
    return s;
}

State* AttachState(lua_State* l, const char* mod) {
    State* s = new State();
    s->state_.l_ = l;
    s->state_.is_attach_ = true;
    s->state_.module_ = mod;

    internal::InitState(s);
    internal::Reg(s);
    internal::g_env.state_list.push_back(std::make_pair(l, s));

    assert(s->GetTop() == 0);
    return s;
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
    struct TypeCreator : public ITypeFactory {
        TypeCreator(const char* name, bool global, const TypeDesc* super)
            : is_global(global), super(super) {
            type_name = AllocTypeName(name);
        }
        virtual ~TypeCreator() { }

        void SetCaster(TypeCaster cast) override {
            caster = cast;
        }

        void SetWeakProc(WeakObjProc proc) override {
            weak_proc = proc;
        }

        void AddMember(bool global, const char* name, LuaFunction func) override {
            name = AllocMemberName(name);
            assert(CheckRename(name, is_global || global));
            ((is_global || global) ? global_funcs : member_funcs).push_back(ExportFunc{name, func});
        }

        void AddMember(bool global, const char* name, LuaIndexer getter, LuaIndexer setter) override {
            name = AllocMemberName(name);
            assert(!Is_G(type_name));                       // _G table not allow variate
            assert(CheckRename(name, is_global || global)); // rename check
            ((is_global || global) ? global_vars : member_vars).push_back(ExportVar{name, getter, setter});
        }

        bool CheckRename(const char* name, bool global) const {
            auto& vars = (global ? global_vars : member_vars);
            for (const auto& v : vars) {
                if (::strcmp(v.name, name) == 0)
                    return false;
            }

            auto& funcs = (global ? global_funcs : member_funcs);
            for (const auto& f : funcs) {
                if (::strcmp(f.name, name) == 0)
                    return false;
            }
            return true;
        }

        const TypeDesc* Finalize() override {
            std::unique_ptr<TypeCreator> hold(this);
            TypeData* data = g_env.allocator.AllocObj<TypeData>();

            data->id = (int)g_env.declared.desc_list.size();
            g_env.declared.desc_list.push_back(data);

            data->name = type_name;
            data->lud_index = GetLudIndex(data);
            data->weak_index = GetWeakIndex();
            data->weak_proc = weak_proc;
            data->caster = caster;
            data->super = super;
            data->child = nullptr;

            // inheritance tree
            if (super) {
                data->brother = super->child;
                const_cast<TypeDesc*>(super)->child = data;
            } else {
                data->brother = nullptr;
            }

            data->member_vars = Alloc(member_vars);
            data->global_vars = Alloc(global_vars);
            data->member_funcs = Alloc(member_funcs);
            data->global_funcs = Alloc(global_funcs);

            // register to exist state
            for (auto pair : g_env.state_list)
                RegDeclared(pair.second, *data);
            return data;
        }

        const char* AllocTypeName(const char* name) {
            char buf[kBuffCacheSize];
            size_t sz = PurifyTypeName(buf, kBuffCacheSize, name);
            return AllocStr(buf, sz);
        }

        const char* AllocMemberName(const char* name) {
            auto view = PurifyMemberName(name);
            return AllocStr(view.str, view.len);
        }

        const char* AllocStr(const char* s, size_t len) {
            char* buf = (char*)g_env.allocator.Alloc(len + 1);
            ::memcpy(buf, s, len + 1);
            return buf;
        }

        TypeVar Alloc(const std::vector<ExportVar>& vars) {
            if (vars.empty())
                return TypeVar{0, nullptr};

            size_t sz = vars.size() * sizeof(ExportVar);
            void* p = g_env.allocator.Alloc(sz);
            ::memcpy(p, &vars[0], sz);
            return TypeVar{vars.size(), (ExportVar*)p};
        }

        TypeFunc Alloc(const std::vector<ExportFunc>& funcs) {
            if (funcs.empty())
                return TypeFunc{0, nullptr};

            size_t sz = funcs.size() * sizeof(ExportFunc);
            void* p = g_env.allocator.Alloc(sz);
            ::memcpy(p, &funcs[0], sz);
            return TypeFunc{funcs.size(), (ExportFunc*)p};
        }

        int GetWeakIndex() const {
            if (is_global || weak_proc.tag == 0)
                return 0;

            auto& tags = g_env.declared.weak_tags;
            auto it = std::find(tags.begin(), tags.end(), weak_proc.tag);
            if (it == tags.end()) {
                tags.push_back(weak_proc.tag);
                return (int)(tags.size() - 1);
            }
            return (int)(it - tags.begin());
        }

        uint8_t GetLudIndex(TypeDesc* desc) const {
            if (is_global)
                return 0;

            if (weak_proc.tag) {
                for (int i = 1; i <= kMaxLudIndex; ++i) {
                    auto& info = g_env.declared.lud_list[i];
                    if (info.type == LudType::Type::kNone) {
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
                        info.desc = desc;
                        return (uint8_t)i;
                    }
                }
            }
            return 0;
        }

        static void* DummyCaster(void* ptr) { return ptr; }

        const char* type_name;
        bool is_global;
        const TypeDesc* super = nullptr;
        TypeCaster caster{&DummyCaster, &DummyCaster};
        WeakObjProc weak_proc{0, nullptr, nullptr};
        std::vector<ExportVar> member_vars;
        std::vector<ExportVar> global_vars;
        std::vector<ExportFunc> member_funcs;
        std::vector<ExportFunc> global_funcs;
    };

    ITypeFactory* CreateFactory(bool global, const char* path, const TypeDesc* super) {
        return new TypeCreator(path, global, super);
    }
}
XLUA_NAMESPACE_END
