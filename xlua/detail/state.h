#pragma once
#include "common.h"
#include "traits.h"
#include "global.h"
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <unordered_map>

XLUA_NAMESPACE_BEGIN

namespace detail {
#if XLUA_ENABLE_LUD_OPTIMIZE
    inline bool IsValidRawPtr(const void* p) {
        return ((0xff00000000000000 & reinterpret_cast<uint64_t>(p)) == 0);
    }

    inline LightUserData MakeLightPtr(void* ptr) {
        LightUserData ud;
        ud.ptr_ = ptr;
        return ud;
    }

    inline LightUserData MakeLightPtr(int type, int index, int serial_num) {
        assert((0xff000000 & index) == 0);
        LightUserData ptr;
        ptr.serial_ = serial_num;
        ptr.index_ = index;
        ptr.type_ = (unsigned char)type;
        return ptr;
    }

    inline LightUserData MakeLightPtr(int type, void* p) {
        assert(IsValidRawPtr(p));
        LightUserData ptr;
        ptr.ptr_ = p;
        ptr.type_ = (unsigned char)type;
        return ptr;
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_weakobj) {
        int index = ::xLuaAllocWeakObjIndex(obj);
        return MakeLightPtr(info->lud_index, index, ::xLuaGetWeakObjSerialNum(index));
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_obj_index) {
        auto& obj_index = obj->xlua_obj_index_;
        auto* ary_obj = GlobalVar::GetInstance()->AllocObjIndex(obj_index, obj, info);
        return MakeLightPtr(info->lud_index, obj_index.GetIndex(), ary_obj->serial_num_);
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_declared) {
        return MakeLightPtr(info->lud_index, obj);
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info) {
        /* 优先弱引用对象 */
        using tag = typename std::conditional<IsWeakObjPtr<Ty>::value, tag_weakobj,
            typename std::conditional<HasObjIndex<Ty>::value, tag_obj_index, tag_declared>::type>::type;
        return MakeLightUserData(obj, info, tag());
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_weakobj) {
        int index = ::xLuaAllocWeakObjIndex(obj);
        return FullUserData(UserDataCategory::kObjPtr, index, ::xLuaGetWeakObjSerialNum(index), info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_obj_index) {
        auto& obj_index = obj->xlua_obj_index_;
        auto* ary_obj = GlobalVar::GetInstance()->AllocObjIndex(obj_index, obj, info);
        return FullUserData(UserDataCategory::kObjPtr, obj_index.GetIndex(), ary_obj->serial_num_, info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_declared) {
        return FullUserData(UserDataCategory::kRawPtr, obj, info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info) {
        /* 优先弱引用对象 */
        using tag = typename std::conditional<IsWeakObjPtr<Ty>::value, tag_weakobj,
            typename std::conditional<HasObjIndex<Ty>::value, tag_obj_index, tag_declared>::type>::type;
        return MakeFullUserData(obj, info, tag());
    }

    template <typename Ty>
    struct ValueUserData : FullUserData {
        ValueUserData(const Ty& val, const TypeInfo* info)
            : FullUserData(UserDataCategory::kValue, nullptr, info)
            , val_(val) {
            obj_ = &val_;
        }

        virtual ~ValueUserData() { }
        void* GetDataPtr() override { return &val_; }

        Ty val_;
    };

    template <typename Ty>
    struct ValueUserData<std::shared_ptr<Ty>> : FullUserData {
        ValueUserData(const std::shared_ptr<Ty>& val, const TypeInfo* info)
            : FullUserData(UserDataCategory::kSharedPtr, nullptr, info)
            , val_(val) {
            obj_ = val_.get();
        }

        virtual ~ValueUserData() {}
        void* GetDataPtr() override { return &val_; }

        std::shared_ptr<Ty> val_;
    };

    struct UserDataInfo {
        bool is_light;
        void* ud;
        void* obj;
        const TypeInfo* info;
    };

    inline bool GetUserDataInfo(lua_State* l, int index, /*out*/UserDataInfo* ret) {
        UserDataInfo ud_info{ false, nullptr, nullptr, nullptr };
        int l_ty = lua_type(l, index);
        if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            ud_info.is_light = true;
            ud_info.ud = lua_touserdata(l, index);
            if (ud_info.ud == nullptr)
                return false;

            auto ud = MakeLightPtr(ud_info.ud);
            if (!ud.IsLightData())
                return false;

            if (ud.IsObjIndex()) {
                auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
                if (ary_obj == nullptr || ary_obj->serial_num_ != ud.serial_)
                    return false;

                ud_info.obj = ary_obj->obj_;
                ud_info.info = ary_obj->info_;
            } else {
                const auto* info = GlobalVar::GetInstance()->GetLUDTypeInfo(ud.type_);
                if (info->is_weak_obj) {
                    if (ud.serial_ != ::xLuaGetWeakObjSerialNum(ud.index_))
                        return false;
                    ud_info.obj = _XLUA_TO_WEAKOBJ_PTR(info, ::xLuaGetWeakObjPtr(ud.index_));
                } else {
                    ud_info.obj = ud.RawPtr();
                }

                ud_info.info = info;
            }
#else
            return false;
#endif // XLUA_ENABLE_LUD_OPTIMIZE
        } else if (l_ty == LUA_TUSERDATA) {
            ud_info.ud = lua_touserdata(l, index);
            if (ud_info.ud == nullptr)
                return false;

            ud_info.is_light = false;
            auto* ud = (FullUserData*)ud_info.ud;
            if (ud->type_ == UserDataCategory::kObjPtr) {
                if (ud->info_->is_weak_obj) {
                    if (ud->index_ != ::xLuaGetWeakObjSerialNum(ud->index_))
                        return false;
                    ud_info.obj = _XLUA_TO_WEAKOBJ_PTR(ud->info_, ::xLuaGetWeakObjPtr(ud->index_));
                } else {
                    auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                    if (ary_obj == nullptr || ary_obj->serial_num_ != ud->serial_)
                        return false;
                    assert(ud->info_ == ary_obj->info_);
                    ud_info.obj = ary_obj->obj_;
                }
            } else {
                /* value or shared_ptr's obj ptr*/
                ud_info.obj = ud->obj_;
            }

            ud_info.info = ud->info_;
        }

        if (ret) *ret = ud_info;
        return true;
    }

    /* user data cache
     * 将User data缓存在一个lua table中, 缓存的key值为对象的基类指针
     * 确保每个对象只构建一个lua user data
    */
    struct UdCache {
        int lua_ref_;
        detail::FullUserData* user_data_;
    };

    /* 引用lua对象(table, function)的引用记录 */
    struct LuaObjRef {
        union {
            int lua_ref_;
            int next_free_;
        };
        int ref_count_;
    };

    struct State {
        template <typename Ty>
        inline LonelyUserData<Ty>* NewLonlyData(const Ty& val) {
            void* mem = lua_newuserdata(state_, sizeof(LonelyUserData<Ty>));
            auto* ud = new (mem) LonelyUserData<Ty>(val);
            lua_rawgeti(state_, LUA_REGISTRYINDEX, lonly_ud_meta_ref_);
            lua_setmetatable(state_, -2);
            return ud;
        }

        template <typename Ty, typename... Args>
        inline Ty* NewUserData(const TypeInfo* info, Args&&... args) {
            void* mem = lua_newuserdata(state_, sizeof(Ty));

            int ty = 0;
            ty = lua_rawgeti(state_, LUA_REGISTRYINDEX, meta_table_ref_);    // type metatable table
            assert(ty == LUA_TTABLE);
            ty = lua_rawgeti(state_, -1, info->index);  // metatable
            assert(ty == LUA_TTABLE);
            lua_remove(state_, -2);                     // remove type table
            lua_setmetatable(state_, -2);
            return new (mem) Ty(std::forward<Args>(args)...);
        }

        void PushPtrUserData(const FullUserData& ud) {
            if (ud.type_ == UserDataCategory::kRawPtr) {
                void* root = GetRootPtr(ud.obj_, ud.info_);
                auto it = raw_ptrs_.find(root);
                if (it != raw_ptrs_.cend()) {
                    UpdateCahce(it->second, ud.obj_, ud.info_);
                    PushUd(it->second);
                } else {
                    UdCache udc = RefCache(NewUserData<FullUserData>(ud.info_, ud));
                    raw_ptrs_.insert(std::make_pair(root, udc));
                }
            } else if (ud.type_ == UserDataCategory::kObjPtr) {
                if (ud.info_->is_weak_obj) {
                    if ((int)weak_obj_ptrs_.size() <= ud.index_) {
                        weak_obj_ptrs_.resize((1 + ud.index_ / XLUA_CONTAINER_INCREMENTAL) * XLUA_CONTAINER_INCREMENTAL, UdCache{0, nullptr});
                    }

                    auto& udc = lua_obj_ptrs_[ud.index_];
                    if (udc.user_data_ != nullptr && udc.user_data_->serial_ == ud.serial_) {
                        UpdateCahce(udc, ud.obj_, ud.info_);
                        PushUd(udc);
                    } else {
                        udc = RefCache(NewUserData<FullUserData>(ud.info_, ud));
                    }
                } else {
                    if ((int)lua_obj_ptrs_.size() <= ud.index_) {
                        lua_obj_ptrs_.resize((1 + ud.index_ / XLUA_CONTAINER_INCREMENTAL) * XLUA_CONTAINER_INCREMENTAL, UdCache{0, nullptr});
                    }

                    auto& udc = lua_obj_ptrs_[ud.index_];
                    if (udc.user_data_ != nullptr && udc.user_data_->serial_ == ud.serial_) {
                        UpdateCahce(udc, ud.obj_, ud.info_);
                        PushUd(udc);
                    } else {
                        udc = RefCache(NewUserData<FullUserData>(ud.info_, ud));
                    }
                }
            } else {
                assert(false);
            }
        }

        template <typename Ty>
        inline void PushValueData(const Ty& val, const detail::TypeInfo* info) {
            NewUserData<detail::ValueUserData<Ty>>(info, val, info);
        }

        template <typename Ty>
        inline void PushSharedPtr(const std::shared_ptr<Ty>& ptr, const detail::TypeInfo* info) {
            void* root = detail::GetRootPtr(ptr.get(), info);
            auto it = shared_ptrs_.find(root);
            if (it != shared_ptrs_.cend()) {
                UpdateCahce(it->second, ptr.get(), info);
                PushUd(it->second);
            } else {
                UdCache udc = RefCache(NewUserData<detail::ValueUserData<std::shared_ptr<Ty>>>(info, ptr, info));
                shared_ptrs_.insert(std::make_pair(root, udc));
            }
        }

        inline void UpdateCahce(UdCache& cache, void* ptr, const detail::TypeInfo* info) {
            if (!detail::IsBaseOf(info, cache.user_data_->info_)) {
                cache.user_data_->obj_ = ptr;
                cache.user_data_->info_ = info;
            }
        }

        inline void PushUd(UdCache& cache) {
            lua_rawgeti(state_, LUA_REGISTRYINDEX, user_data_table_ref_);
            lua_rawgeti(state_, -1, cache.lua_ref_);
            lua_remove(state_, -2);
        }

        inline UdCache RefCache(detail::FullUserData* data) {
            UdCache udc ={0, data};
            lua_rawgeti(state_, LUA_REGISTRYINDEX, user_data_table_ref_);
            lua_pushvalue(state_, -2);
            udc.lua_ref_ = luaL_ref(state_, -2);
            lua_pop(state_, 1);
            return udc;
        }

        inline void UnRefCachce(UdCache& cache) {
            lua_rawgeti(state_, LUA_REGISTRYINDEX, user_data_table_ref_);
            luaL_unref(state_, -1, cache.lua_ref_);
            lua_pop(state_, 1);
            cache.lua_ref_ = 0;
            cache.user_data_ = nullptr;
        }

        void Gc(FullUserData* ud) {
            switch (ud->type_) {
            case detail::UserDataCategory::kValue:
                break;
            case detail::UserDataCategory::kRawPtr:
            {
                auto it = raw_ptrs_.find(detail::GetRootPtr(ud->obj_, ud->info_));
                UnRefCachce(it->second);
                raw_ptrs_.erase(it);
            }
            break;
            case detail::UserDataCategory::kSharedPtr:
            {
                auto it = shared_ptrs_.find(detail::GetRootPtr(ud->obj_, ud->info_));
                UnRefCachce(it->second);
                shared_ptrs_.erase(it);
            }
            break;
            case detail::UserDataCategory::kObjPtr:
                if (ud->info_->is_weak_obj) {
                    auto& udc = weak_obj_ptrs_[ud->index_];
                    if (ud == udc.user_data_)
                        UnRefCachce(udc);
                } else {
                    auto& udc = lua_obj_ptrs_[ud->index_];
                    if (ud == udc.user_data_)
                        UnRefCachce(udc);
                }
                break;
            default:
                assert(false);
                break;
            }

            ud->~FullUserData();
        }

        int RefLuaObj(int index) {
            if (next_free_lua_obj_ == -1) {
                size_t o_s = lua_objs_.size();
                size_t n_s = o_s + XLUA_CONTAINER_INCREMENTAL;
                lua_objs_.resize(n_s, LuaObjRef{-1, 0});

                for (size_t i = o_s; i < n_s; ++i)
                    lua_objs_[i].next_free_ = (int)(i + 1);
                next_free_lua_obj_ = (int)o_s;
                lua_objs_.back().next_free_ = -1;
            }

            int ary_idx = next_free_lua_obj_;
            auto& ref = lua_objs_[ary_idx];
            next_free_lua_obj_ = ref.next_free_;

            ref.lua_ref_ = Ref(index);
            ref.ref_count_ = 1;
            return ary_idx;
        }

        inline void LoadLuaObj(int ary_index) {
            assert(ary_index >= 0 && ary_index < (int)lua_objs_.size());
            LoadRef(lua_objs_[ary_index].lua_ref_);
        }

        inline void AddLuaObjRef(int ary_index) {
            assert(ary_index >= 0 && ary_index < (int)lua_objs_.size());
            ++lua_objs_[ary_index].ref_count_;
        }

        inline void ReleaseLuaObj(int ary_index) {
            assert(ary_index >= 0 && ary_index < (int)lua_objs_.size());
            auto& ref = lua_objs_[ary_index];
            -- ref.ref_count_;
            if (ref.ref_count_ == 0) {
                UnRef(ref.lua_ref_);
                ref.next_free_ = next_free_lua_obj_;
                next_free_lua_obj_ = ary_index;
            }
        }

        inline int Ref(int index) {
            index = lua_absindex(state_, index);
            lua_rawgeti(state_, LUA_REGISTRYINDEX, lua_obj_table_ref_); // load table
            lua_pushvalue(state_, index);           // copy value
            int ref = luaL_ref(state_, -2);         // ref value
            lua_pop(state_, 1);                     // pop table
            return ref;
        }

        inline void UnRef(int ref) {
            lua_rawgeti(state_, LUA_REGISTRYINDEX, lua_obj_table_ref_);
            luaL_unref(state_, -1, ref);
            lua_pop(state_, 1);
        }

        inline void LoadRef(int ref) {
            lua_rawgeti(state_, LUA_REGISTRYINDEX, lua_obj_table_ref_);
            lua_rawgeti(state_, -1, ref);
            lua_remove(state_, -2);
        }

        bool attach_;
        lua_State* state_;
        int meta_table_ref_ = 0;        // 导出元表索引
        int user_data_table_ref_ = 0;   // user data table
        int lua_obj_table_ref_ = 0;     // table, function
        int lonly_ud_meta_ref_ = 0;     // 独立user data元表索引
        int type_meta_func_ref_ = 0;
        int next_free_lua_obj_ = -1;    // 下一个的lua对象表空闲槽
        std::vector<LuaObjRef> lua_objs_;
        std::vector<UdCache> lua_obj_ptrs_;
        std::vector<UdCache> weak_obj_ptrs_;
        std::unordered_map<void*, UdCache> raw_ptrs_;
        std::unordered_map<void*, UdCache> shared_ptrs_;
        char type_name_buf_[XLUA_MAX_TYPE_NAME_LENGTH];       // 输出类型名称缓存
    };

    /* 引用Lua对象基类
     * Lua对象主要指Table, Function
    */
    struct ObjBase {
        ObjBase() : ary_index_(-1), state_(nullptr) {}
        ObjBase(State* state, int index) : state_(state), ary_index_(index) {}
        ~ObjBase() { UnRef(); }

        ObjBase(const ObjBase& other)
            : state_(other.state_)
            , ary_index_(other.ary_index_) {
            AddRef();
        }

        ObjBase(ObjBase&& other)
            : state_(other.state_)
            , ary_index_(other.ary_index_) {
            other.ary_index_ = -1;
        }

        inline bool IsValid() const { return ary_index_ != -1; }

        ObjBase& operator = (const ObjBase& other) {
            UnRef();
            ary_index_ = other.ary_index_;
            state_ = other.state_;
            AddRef();
            return *this;
        }

        ObjBase& operator = (ObjBase&& other) {
            UnRef();
            state_ = other.state_;
            ary_index_ = other.ary_index_;
            other.ary_index_ = -1;
            return *this;
        }

        inline bool operator == (const ObjBase& other) const {
            return ary_index_ == other.ary_index_;
        }

        inline void AddRef() {
            if (ary_index_ != -1)
                state_->AddLuaObjRef(ary_index_);
        }

        inline void UnRef() {
            if (ary_index_ != -1) {
                state_->ReleaseLuaObj(ary_index_);
                ary_index_ = -1;
            }
        }

        State* state_;
        int ary_index_;
    };

} // namespace detail

XLUA_NAMESPACE_END
