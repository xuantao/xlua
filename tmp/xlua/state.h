#pragma once
#include "common.h"
#include <array>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <lua.hpp>


XLUA_NAMESPACE_BEGIN
#if XLUA_ENABLE_LUD_OPTIMIZE
    #define _IS_TABLE_TYPE(type)    (type == LUA_TTABLE || type == LUA_TLIGHTUSERDATA || type == LUA_TUSERDATA)
#else
    #define _IS_TABLE_TYPE(type)    (type == LUA_TTABLE || type == LUA_TFullData)
#endif

namespace internal {
    constexpr size_t kMaxLudIndex1 = 0xff;
    constexpr size_t kMaxLudIndex = 0x00ffffff;

    inline bool IsBaseOf(const TypeDesc* base, const TypeDesc* drive) {
        while (drive && drive != base)
            drive = drive->super;
        return drive == base;
    }

    inline const TypeDesc* GetSuperDesc(const TypeDesc* dest) {
        while (dest->super != nullptr)
            dest = dest->super;
        return dest;
    }

    inline bool IsUd(FullData* ud, const TypeDesc* desc) {
        if (ud->dc != UdType::kDeclaredType)
            return false;
        if (!IsBaseOf(desc, ud->desc))
            return false;
        return true;
    }

    inline bool IsUd(FullData* ud, ICollection* collection) {
        return ud->dc == UdType::kCollection && ud->collection == collection;
    }

    inline void* As(FullData* ud, const TypeDesc* desc) {
        if (!IsUd(ud, desc))
            return nullptr;

        // check the weak obj ref is valid
        if (ud->vc == UvType::kPtr && ud->desc->weak_index > 0) {
            if (ud->desc->weak_proc.getter(ud->ref) == nullptr)
                return nullptr;
            //auto wd = GetWeakObjData(ud->desc->weak_index, ud->ref.index);
            //return _XLUA_TO_SUPER_PTR(wd.obj, wd.desc, desc);
        }
        return _XLUA_TO_SUPER_PTR(ud->obj, ud->desc, desc);
    }

    inline void* As(FullData* ud, ICollection* desc) {
        if (!IsUd(ud, desc))
            return nullptr;
        return ud->obj;
    }

    template <typename Ty, typename std::enable_if<std::is_base_of<object_category_tag_, typename Support<Ty>::category>::value, int>::type = 0>
    inline Ty* As(FullData* ud) {
        if (ud->tag_1_ != _XLUA_TAG_1 || ud->tag_2_ != _XLUA_TAG_2)
            return nullptr;
        return static_cast<Ty*>(As_(ud, Support<Ty>::Desc()));
    }

    inline bool IsValid(const FullData* ud) {
        return ud->tag_1_ == _XLUA_TAG_1 && ud->tag_2_ == _XLUA_TAG_2;
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    struct WeakObjCache {
        void* obj;
        const TypeDesc* desc;
    };

    struct LightData {
        //TODO: bigedian?
        union {
            // raw ptr
            struct {
                int64_t lud_index : 8;
                int64_t ptr : 56;
            };
            // weak obj ref
            struct {
                int64_t weak_index : 8;
                int64_t ref_index : 24;
                int64_t ref_serial : 32;
            };
            // none
            struct {
                void* value_;
            };
        };

        inline void* ToObj() const {
            return reinterpret_cast<void*>(ptr);
        }
        inline WeakObjRef ToWeakRef() const {
            return WeakObjRef{(int)ref_index, (int)ref_serial};
        }

        static inline LightData Make(int8_t weak_index, int32_t ref_index, int32_t ref_serial) {
            LightData ld;
            ld.weak_index = weak_index;
            ld.ref_index = ref_index;
            ld.ref_serial = ref_serial;
            return ld;
        }

        static inline LightData Make(int8_t lud_index, void* p) {
            LightData ld;
            ld.value_ = p;
            ld.lud_index = lud_index;
            return ld;
        }

        static inline LightData Make(void* p) {
            LightData ld;
            ld.value_ = p;
            return ld;
        }

        static inline bool IsValid(void* p) {
            auto val = reinterpret_cast<uint64_t>(p);
            return (val & 0xff00000000000000) != 0;
        }
    };

    const TypeDesc* GetTypeDesc(int lud_index);
    WeakObjCache GetWeakObjData(int weak_index, int obj_index);
    void SetWeakObjData(int weak_idnex, int obj_index, void* obj, const TypeDesc* desc);
    int GetMaxWeakIndex();

    inline bool IsUd(LightData ld, const TypeDesc* desc) {
        if (ld.lud_index == 0)
            return false;

        if (ld.lud_index <= GetMaxWeakIndex()) {
            auto data = GetWeakObjData(ld.weak_index, ld.ref_index);
            return data.desc != nullptr && IsBaseOf(desc, data.desc);
        } else {
            const auto* ud_desc = GetTypeDesc(ld.lud_index);
            return ud_desc != nullptr && IsBaseOf(desc, ud_desc);
        }
    }

    inline bool IsUd(LightData ld, ICollection* collection) {
        return false;
    }

    inline void* As(LightData ld, const TypeDesc* desc) {
        if (ld.lud_index == 0)
            return nullptr;

        if (ld.lud_index <= GetMaxWeakIndex()) {
            auto data = GetWeakObjData(ld.weak_index, ld.ref_index);
            if (data.desc == nullptr || !IsBaseOf(desc, data.desc))
                return nullptr;
            if (data.desc->weak_proc.getter(ld.ToWeakRef()) == nullptr)
                return nullptr;
            return _XLUA_TO_SUPER_PTR(data.obj, data.desc, desc);
        } else {
            const auto* ud_desc = GetTypeDesc(ld.lud_index);
            if (ud_desc == nullptr || !IsBaseOf(desc, ud_desc))
                return nullptr;
            return _XLUA_TO_SUPER_PTR(ld.ToObj(), ud_desc, desc);
        }
    }

    inline void* As(LightData ld, ICollection* collection) {
        return nullptr;
    }

    template <typename Ty>
    inline Ty* As(LightData ld) {
        return static_cast<Ty*>(As(ld, Support<Ty>::Desc()));
    }

    template <typename Ty>
    inline void* MakeLightPtr(Ty* obj, const TypeDesc* desc) {
        if (desc->weak_index && desc->weak_index <= XLUA_MAX_WEAKOBJ_TYPE_COUNT) {
            WeakObjRef ref = desc->weak_proc.maker(obj);
            if (ref.index <= kMaxLudIndex)
                return LightData::Make(desc->weak_index, ref.index, ref.serial).value_;
        } else if (desc->lud_index && desc->lud_index <= 0xff) {
            if (LightData::IsValid(obj))
                return LightData::Make(desc->lud_index, obj).value_;
        }
        return nullptr;
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    struct UdCache {
        int ref;
        void* ud;
    };

    struct LuaObjArray {
        struct ObjRef {
            union {
                int ref;
                int next;
            };
            int count;
        };

        int empty = 0;
        std::vector<ObjRef> objs{ ObjRef() };   // first element is not used
    };

    struct StateData {
        int LoadGolbal(const char* path) {
            if (path == nullptr || path[0] == 0) {
                lua_pushglobaltable(l_);
                return LUA_TTABLE;
            }

            lua_pushglobaltable(l_);
            while (const char* sub = ::strchr(path, '.')) {
                lua_pushlstring(l_, path, sub - path);
                if (lua_gettable(l_, -2) != LUA_TTABLE) {
                    lua_pop(l_, 2);
                    lua_pushnil(l_);
                    return LUA_TNIL;
                }

                lua_remove(l_, -2);
                path = sub + 1;
            }

            int ty = lua_getfield(l_, -1, path);
            lua_remove(l_, -2);
            return ty;
        }

        bool SetGlobal(const char* path, bool create, bool rawset) {
            int top = lua_gettop(l_);
            if (top == 0)
                return false;           // none value

            if (path == nullptr || *path == 0) {
                lua_pop(l_, 1);         // pop value
                return false;           // wrong name
            }

            lua_pushglobaltable(l_);
            while (const char* sub = ::strchr(path, '.')) {
                lua_pushlstring(l_, path, sub - path);
                int l_ty = lua_gettable(l_, -2);
                if (l_ty == LUA_TNIL && create) {
                    lua_pop(l_, 1);                         // pop nil
                    lua_newtable(l_);                       // new table
                    lua_pushlstring(l_, path, sub - path);  // key
                    lua_pushvalue(l_, -2);                  // copy table
                    if (rawset)
                        lua_rawset(l_, -4);
                    else
                        lua_settable(l_, -4);               // set field
                }
                else if (!_IS_TABLE_TYPE(l_ty)) {
                    lua_pop(l_, 3);                         // [1]:value, [2]:table, [3]:unknown value
                    return false;
                }

                lua_remove(l_, -2);
                path = sub + 1;
            }

            lua_pushstring(l_, path);
            lua_rotate(l_, top, -1);
            if (rawset)
                lua_rawset(l_, -3);
            else
                lua_settable(l_, -3);    // set field

            lua_pop(l_, 1);              // [1]: table
            return true;
        }

        FullData* LoadRawUd(int index) {
            if (lua_type(l_, index) != LUA_TUSERDATA)
                return nullptr;

            auto* ud = static_cast<FullData*>(lua_touserdata(l_, index));
            if (ud->tag_1_ != _XLUA_TAG_1 || ud->tag_2_ != _XLUA_TAG_2)
                return nullptr;
            return ud;
        }

        template <typename Ty>
        inline bool IsUd(int index) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index)) {
                return IsUd(LightData::Make(lua_touserdata(l_, index)),
                    Support<Ty>::Desc());
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadRawUd(index);
            if (ud == nullptr)
                return false;
            return IsUd(ud, Support<Ty>::Desc());
        }

        template <typename Ty>
        Ty* LoadUd(int index) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index)) {
                return As<Ty>(LightData::Make(lua_touserdata(l_, index)));
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadRawUd(index);
            if (ud == nullptr)
                return nullptr;
            return As<Ty>(ud);
        }

        // value
        template <typename Ty>
        inline void PushUd(const Ty& obj) {
            NewUserData<ValueData<Ty>>(obj);
            SetMetatable(Support<Ty>::Desc());
        }

        // collection ptr
        template <typename Ty, typename std::enable_if<std::is_same<collection_category_tag, typename Support<Ty>::category>::value, int>::type = 0>
        inline void PushUd(Ty* ptr) {
            auto it = collection_ptrs_.find(static_cast<void*>(ptr));
            if (it == collection_ptrs_.end()) {
                NewUserData<FullData>(ptr);
                SetMetatable(static_cast<ICollection*>(nullptr));
                collection_ptrs_.insert(std::make_pair(ptr, CacheUd()));
            } else {
                LoadCache(it->second.ref);
            }
        }

        // delcared type ptr
        template <typename Ty, typename std::enable_if<std::is_same<declared_category_tag, typename Support<Ty>::category>::value, int>::type = 0>
        inline void PushUd(Ty* ptr) {
            if (ptr == nullptr) {
                lua_pushnil(l_);
                return;
            }

            const auto* desc = Support<Ty>::Desc();
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (void* lud = MakeLightPtr(ptr, desc)) {
                lua_pushlightuserdata(l_, lud);
                return;
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

            auto* tsp = _XLUA_TO_TOP_SUPER_PTR(ptr, desc);
            auto it = declared_ptrs_.find(tsp);
            if (it == declared_ptrs_.end()) {
                NewUserData<FullData>(ptr);
                SetMetatable(desc);
                declared_ptrs_.insert(std::make_pair(tsp, CacheUd()))
            } else {
                LoadCache(it->second.ref);
                if (!IsBaseOf(desc, it->second.ud->desc))
                    SetMetatable(desc);
            }
        }

        // smart ptr
        template <typename Sty, typename Ty>
        inline void PushSmartPtr(const Sty& s, Ty* ptr, size_t tag) {
            const auto* desc = Support<Ty>::Desc();
            auto* tsp = _XLUA_TO_TOP_SUPER_PTR(ptr, desc);
            auto it = smart_ptrs_.find(tsp);
            if (it == smart_ptrs_.end()) {
                NewUserData<SmartPtrDataImpl<Ty>>(s, ptr, tag);
                SetMetatable(desc);
                smart_ptrs_.insert(std::make_pair(tsp, CacheUd()));
            } else {
                assert(static_cast<SmartPtrData*>(it->second.ud)->tag == tag);
                LoadCache(it->second.ref);
                if (!IsBaseOf(desc, it->second.ud->desc))
                    SetMetatable(desc);
            }
        }

        template <typename Ty, typename... Args>
        inline typename std::enable_if<std::is_base_of<FullData, Ty>::value, Ty*>::type NewUserData(Args&&... args) {
            void* d = lua_newuserdata(l_, sizeof(Ty));
            return new (d) Ty(std::forward<Args>(args)...);
        }

        inline void SetMetatable(const TypeDesc* desc) {
            lua_geti(l_, LUA_REGISTRYINDEX, meta_ref_); // load metatable_list
            lua_geti(l_, -1, desc->id);                 // get type metatable
            lua_remove(l_, -2);                         // remove metatable_list
            lua_setmetatable(l_, -2);                   // set metatable
        }

        inline void SetMetatable(ICollection*) {
            lua_geti(l_, LUA_REGISTRYINDEX, collection_meta_ref_);  // load collection metatable
            lua_setmetatable(l_, -2);                               // set metatable
        }

        int RefObj(int index) {
            int lty = lua_type(l_, index);
            //TODO: lua thread?
            if (lty != LUA_TFUNCTION && lty != LUA_TUSERDATA && lty != LUA_TFUNCTION)
                return 0;

            // reference lua object
            int ref = 0;
            index = lua_absindex(l_, index);
            lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
            lua_pushvalue(l_, index);                       // copy data
            ref = luaL_ref(l_, -2);                         // ref top stack user data
            lua_remove(l_, -1);                             // remove cache table

            // record reference count
            if (obj_ary_.empty == 0) {
                size_t sz = obj_ary_.objs.size();
                size_t ns = sz + 4096;

                obj_ary_.objs.resize(ns);
                for (size_t i = ns; i > sz; --i) {
                    auto& o = obj_ary_.objs[i - 1];
                    o.next = (int)i;
                    o.count = 0;
                }
                obj_ary_.objs[ns].next = 0;
            }

            int obj_idx = obj_ary_.empty;
            auto& obj = obj_ary_.objs[obj_idx];
            obj_ary_.empty = obj.next;
            obj.ref = ref;
            obj.count = 1;
            return obj_idx;;
        }

        inline bool LoadRef(int obj_idx) {
            if (obj_idx <= 0 || obj_idx >= (int)obj_ary_.objs.size())
                return false;

            lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
            lua_geti(l_, -1, obj_ary_.objs[obj_idx].ref);   // load lua obj
            lua_remove(l_, -2);                             // remove obj table
            return true;
        }

        void AddObjRef(int obj_idx) {
            if (obj_idx <= 0 || obj_idx >= (int)obj_ary_.objs.size())
                return;

            auto& obj = obj_ary_.objs[obj_idx];
            assert(obj.count);
            ++obj.count;
        }

        void DecObjRef(int obj_idx) {
            if (obj_idx <= 0 || obj_idx >= (int)obj_ary_.objs.size())
                return;

            auto& obj = obj_ary_.objs[obj_idx];
            assert(obj.count);
            --obj.count;

            if (obj.count == 0) {
                obj.next = obj_ary_.empty;
                obj_ary_.empty = obj_idx;
            }
        }

        /* cache the user data on lua top stack */
        inline UdCache CacheUd() {
            int ref = 0;
            auto* ud = static_cast<FullData*>(lua_touserdata(l_, -1));
            ASSERT_FUD(ud);

            lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
            lua_pushvalue(l_, -2);                          // copy user data to top
            ref = luaL_ref(l_, -2);                         // ref top stack user data
            lua_remove(l_, -1);                             // remove cache table

            return UdCache{ref, ud};
        }

        inline void LoadCache(int ref) {
            lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
            lua_geti(l_, -1, ref);                          // load cache data
            lua_remove(l_, -2);                             // remove cache table
        }

        /* user data gc */
        void OnGc(FullData* ud) {
            UdCache ch{0, nullptr};
            if (ud->vc == UvType::kPtr) {
                if (ud->dc == UdType::kCollection) {
                    auto it = collection_ptrs_.find(ud->obj);
                    if (it != collection_ptrs_.end()) {
                        ch = it->second;
                        collection_ptrs_.erase(it);
                    }
                } else if (ud->dc == UdType::kDeclaredType) {
                    auto it = declared_ptrs_.find(ud->obj);
                    if (it != declared_ptrs_.end()) {
                        ch = it->second;
                        declared_ptrs_.erase(it);
                    }
                }
            } else if (ud->vc == UvType::kSmartPtr) {
                auto it = smart_ptrs_.find(ud->obj);
                if (it != smart_ptrs_.end()) {
                    ch = it->second;
                    smart_ptrs_.erase(it);
                }

                static_cast<ObjData*>(ud)->~ObjData();
            } else {
                static_cast<ObjData*>(ud)->~ObjData();
            }

            if (ch.ref) {
                lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
                luaL_unref(l_, -1, ch.ref);                     // unref cache data
                lua_pop(l_, 1);                                 // remove cache table
            }
        }

        lua_State* l_;
        int desc_ref_;
        int meta_ref_;
        int collection_meta_ref_;
        int obj_ref_;
        int cache_ref_;

        /* ref lua objects, such as table, function, user data*/
        LuaObjArray obj_ary_;
        /* */
        std::unordered_map<void*, UdCache> collection_ptrs_;
        std::unordered_map<void*, UdCache> declared_ptrs_;
        std::unordered_map<void*, UdCache> smart_ptrs_;
    };

} // internal

XLUA_NAMESPACE_END
