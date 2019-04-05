#pragma once
#include "../xlua_def.h"
#include "traits.h"
#include "global.h"
#include <memory>
#include <unordered_map>

XLUA_NAMESPACE_BEGIN

namespace detail {
    inline const TypeInfo* GetRootType(const TypeInfo* info) {
        while (info->super)
            info = info->super;
        return info;
    }

    inline void* GetRootPtr(void* ptr, const TypeInfo* info) {
        const TypeInfo* root = GetRootType(info);
        if (info == root)
            return ptr;
        return _XLUA_TO_SUPER_PTR(root, ptr, info);
    }

    inline bool IsBaseOf(const TypeInfo* base, const TypeInfo* info) {
        while (info && info != base)
            info = info->super;
        return info == base;
    }

    template <typename Ty>
    inline xLuaIndex& GetRootObjIndex(Ty* obj) {
        return static_cast<typename InternalRoot<Ty>::type*>(obj)->xlua_obj_index_;
    }

#ifdef XLUA_USE_LIGHT_USER_DATA
    struct LightUserData {
        union {
            struct {
                void* ptr_;
            };
            struct {
                uint32_t serial_;
                uint32_t index_ : 24;
                uint32_t type_ : 8;
            };
        };

        inline void* Ptr() const {
            return ptr_;
        }

        inline void* RawPtr() const {
            return (void*)(reinterpret_cast<uint64_t>(ptr_) & 0x00ffffffffffffff);
        }

        inline void SetRawPtr(void* p) {
            ptr_ = (void*)((reinterpret_cast<uint64_t>(ptr_) & 0xff000000000000)
                | (reinterpret_cast<uint64_t>(p) & 0x00ffffffffffffff));
        }
    };

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
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_internal) {
        auto& obj_index = GetRootObjIndex(obj);
        auto* ary_obj = GlobalVar::GetInstance()->AllocObjIndex(obj_index, obj, info);
        return MakeLightPtr(0, obj_index.GetIndex(), ary_obj->serial_num_);
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_weakobj) {
        int index = ::xLuaAllocWeakObjIndex(obj);
        return MakeLightPtr(info->external_type_index, index, ::xLuaGetWeakObjSerialNum(index));
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info, tag_external) {
        return MakeLightPtr(info->external_type_index, obj);
    }

    template <typename Ty>
    inline LightUserData MakeLightUserData(Ty* obj, const TypeInfo* info) {
        using tag = typename std::conditional<IsInternal<Ty>::value, tag_internal,
            typename std::conditional<IsWeakObjPtr<Ty>::value, tag_weakobj, tag_external>::type>::type;
        return MakeLightUserData(obj, info, tag());
    }
#endif // XLUA_USE_LIGHT_USER_DATA

    enum class UserDataCategory {
        kValue,
        kRawPtr,
        kSharedPtr,
        kObjPtr,
    };

    struct FullUserData {
        FullUserData(UserDataCategory type, void* obj, const TypeInfo* info)
            : type_(type)
            , info_(info)
            , obj_(obj) {
        }

        FullUserData(UserDataCategory type, int32_t index, int32_t serial, const TypeInfo* info)
            : type_(type)
            , info_(info)
            , index_(index)
            , serial_(serial) {
        }

        virtual ~FullUserData() {}
        virtual void* GetDataPtr() { return nullptr; }

        UserDataCategory type_;
        const TypeInfo* info_;
        union {
            void* obj_;
            struct {
                int32_t index_;
                int32_t serial_;
            };
        };
    };

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_internal) {
        auto& obj_index = GetRootObjIndex(obj);
        auto* ary_obj = GlobalVar::GetInstance()->AllocObjIndex(obj_index, obj, info);
        return FullUserData(UserDataCategory::kObjPtr, obj_index.GetIndex(), ary_obj->serial_num_, info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_weakobj) {
        int index = ::xLuaAllocWeakObjIndex(obj);
        return FullUserData(UserDataCategory::kObjPtr, index, ::xLuaGetWeakObjSerialNum(index), info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info, tag_external) {
        return FullUserData(UserDataCategory::kRawPtr, obj, info);
    }

    template <typename Ty>
    inline FullUserData MakeFullUserData(Ty* obj, const TypeInfo* info) {
        using tag = typename std::conditional<IsInternal<Ty>::value, tag_internal,
            typename std::conditional<IsWeakObjPtr<Ty>::value, tag_weakobj, tag_external>::type>::type;
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

    template <typename Ty>
    Ty* GetLightUserDataPtr(void* user_data, const TypeInfo* info) {
#if XLUA_USE_LIGHT_USER_DATA
        LightUserData ud = MakeLightPtr(user_data);
        if (ud.type_ == 0) {
            ArrayObj* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
            if (ary_obj == nullptr || ary_obj->serial_num_ != ud.serial_)
                return nullptr;

            if (!IsBaseOf(info, ary_obj->info_)) {
                LogError("can not get obj of type:[%s] from:[%s]", info->type_name, ary_obj->info_->type_name);
                return nullptr;
            }

            return (Ty*)_XLUA_TO_SUPER_PTR(info, ary_obj->obj_, ary_obj->info_);
        } else {
            const TypeInfo* dst_type = GlobalVar::GetInstance()->GetExternalTypeInfo(ud.type_);
            if (!IsBaseOf(info, dst_type)) {
                LogError("can not get obj of type:[%s] from:[%s]", info->type_name, dst_type->type_name);
                return nullptr;
            }

            if (dst_type->is_weak_obj) {
                if (ud.serial_ != ::xLuaGetWeakObjSerialNum(ud.index_))
                    return nullptr;
                return (Ty*)_XLUA_TO_WEAKOBJ_PTR(dst_type, ::xLuaGetWeakObjPtr(ud.index_));
            } else {
                return (Ty*)_XLUA_TO_SUPER_PTR(dst_type, ud.RawPtr(), info);
            }
        }
#endif // XLUA_USE_LIGHT_USER_DATA
        return nullptr;
    }

    template <typename Ty>
    Ty* GetFullUserDataPtr(void* p, const TypeInfo* info) {
        FullUserData* ud = static_cast<FullUserData*>(p);
        if (ud->obj_ == nullptr)
            return nullptr;
        if (!IsBaseOf(info, ud->info_))
            return nullptr;

        if (ud->type_ == UserDataCategory::kRawPtr) {
            return (Ty*)_XLUA_TO_SUPER_PTR(info, ud->obj_, ud->info_);
        }

        if (ud->type_ == UserDataCategory::kObjPtr) {
            if (ud->info_->is_weak_obj) {
                auto base = (XLUA_WEAK_OBJ_BASE_TYPE*)::xLuaGetWeakObjPtr(ud->index_);
                if (base == nullptr || ud->serial_ != ::xLuaGetWeakObjSerialNum(ud->index_))
                    return nullptr;
                return static_cast<Ty*>(base);
            } else {
                ArrayObj* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                if (ary_obj == nullptr || ary_obj->obj_ == nullptr || ary_obj->serial_num_ != ud->serial_)
                    return nullptr;
                return (Ty*)_XLUA_TO_SUPER_PTR(info, ary_obj->obj_, ud->info_);
            }
        }

        //TODO: log unknown obj type
        return nullptr;
    }
} // namespace detail

XLUA_NAMESPACE_END
