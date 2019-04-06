#pragma once
#include "common.h"
#include "traits.h"
#include "global.h"
#include <memory>
#include <unordered_map>

XLUA_NAMESPACE_BEGIN

namespace detail {
#ifdef XLUA_USE_LIGHT_USER_DATA
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
        assert((0xff00000000000000 & reinterpret_cast<uint64_t>(p)) == 0);
        LightUserData ptr;
        ptr.ptr_ = p;
        ptr.type_ = (unsigned char)type;
        return ptr;
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_weakobj) {
        int index = ::xLuaAllocWeakObjIndex(obj);
        return MakeLightPtr(info->external_type_index, index, ::xLuaGetWeakObjSerialNum(index));
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_internal) {
        auto& obj_index = GetRootObjIndex(obj);
        auto* ary_obj = GlobalVar::GetInstance()->AllocObjIndex(obj_index, obj, info);
        return MakeLightPtr(0, obj_index.GetIndex(), ary_obj->serial_num_);
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_external) {
        return MakeLightPtr(info->external_type_index, obj);
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info) {
        /* 优先弱引用对象 */
        using tag = typename std::conditional<IsWeakObjPtr<Ty>::value, tag_weakobj,
            typename std::conditional<IsInternal<Ty>::value, tag_internal, tag_external>::type>::type;
        return MakeLightUserData(obj, info, tag());
    }
#endif // XLUA_USE_LIGHT_USER_DATA

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_weakobj) {
        int index = ::xLuaAllocWeakObjIndex(obj);
        return FullUserData(UserDataCategory::kObjPtr, index, ::xLuaGetWeakObjSerialNum(index), info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_internal) {
        auto& obj_index = GetRootObjIndex(obj);
        auto* ary_obj = GlobalVar::GetInstance()->AllocObjIndex(obj_index, obj, info);
        return FullUserData(UserDataCategory::kObjPtr, obj_index.GetIndex(), ary_obj->serial_num_, info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_external) {
        return FullUserData(UserDataCategory::kRawPtr, obj, info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info) {
        /* 优先弱引用对象 */
        using tag = typename std::conditional<IsWeakObjPtr<Ty>::value, tag_weakobj,
            typename std::conditional<IsInternal<Ty>::value, tag_internal, tag_external>::type>::type;
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
        int l_ty = lua_type(l, 1);
        if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_USE_LIGHT_USER_DATA
            ud_info.is_light = true;
            ud_info.ud = lua_touserdata(l, 1);
            if (ud_info.ud == nullptr)
                return false;

            auto ud = MakeLightPtr(ud_info.ud);
            if (ud.type_ != 0) {
                const auto* info = GlobalVar::GetInstance()->GetExternalTypeInfo(ud.type_);
                if (info->is_weak_obj) {
                    if (ud.serial_ != ::xLuaGetWeakObjSerialNum(ud.index_))
                        return false;
                    ud_info.obj = _XLUA_TO_WEAKOBJ_PTR(info, ::xLuaGetWeakObjPtr(ud.index_));
                } else {
                    ud_info.obj = ud.RawPtr();
                }

                ud_info.info = info;
            } else {
                auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
                if (ary_obj == nullptr || ary_obj->serial_num_ != ud.serial_)
                    return false;

                ud_info.obj = ary_obj->obj_;
                ud_info.info = ary_obj->info_;
            }
#else
            return false;
#endif // XLUA_USE_LIGHT_USER_DATA
        } else if (l_ty == LUA_TUSERDATA) {
            ud_info.ud = lua_touserdata(l, 1);
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
