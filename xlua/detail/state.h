#pragma once
#include "common.h"
#include "traits.h"
#include "global.h"
#include <memory>
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
} // namespace detail

XLUA_NAMESPACE_END
