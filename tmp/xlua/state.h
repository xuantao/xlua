#pragma once
#include "common.h"
#include <array>
#include <vector>
#include <unordered_map>
#include <lua.hpp>

XLUA_NAMESPACE_BEGIN

namespace internal {
    inline bool IsBaseOf(const TypeDesc* base, const TypeDesc* drive) {
        while (drive && drive != base)
            drive = drive->super;
        return drive == base;
    }

    inline const TypeDesc* ToSuperBase(const TypeDesc* dest) {
        while (dest->super != nullptr)
            dest = dest->super;
        return dest;
    }

    inline bool IsUd(UserData* ud, const TypeDesc* desc) {
        if (ud->dc != UdType::kDeclaredType)
            return false;
        if (!IsBaseOf(desc, ud->desc))
            return false;
        return true;
    }

    inline bool IsUd(UserData* ud, ICollection* collection) {
        return ud->dc == UdType::kCollection && ud->collection == collection;
    }

    inline void* As(UserData* ud, const TypeDesc* desc) {
        if (!IsUd(ud, desc))
            return nullptr;
        // check the weak obj ref is valid
        if (ud->vc == UvType::kPtr && ud->desc->weak_index != 0) {
            auto* wud = static_cast<WeakRefData*>(ud);
            if (ud->desc->weak_proc.getter(wud->ref) == nullptr)
                return nullptr;
        }
        return _XLUA_TO_SUPER_PTR(ud->obj, ud->desc, desc);
    }

    inline void* As(UserData* ud, ICollection* desc) {
        if (!IsUd(ud, desc))
            return nullptr;
        return ud->obj;
    }

    template <typename Ty, typename std::enable_if<std::is_base_of<ObjectCategory_, typename Support<Ty>::category>::value, int>::type = 0>
    inline Ty* As(UserData* ud) {
        if (ud->tag_1_ != _XLUA_TAG_1 || ud->tag_2_ != _XLUA_TAG_2)
            return nullptr;
        return static_cast<Ty*>(As_(ud, Support<Ty>::Desc()));
    }

    inline bool IsValid(const UserData* ud) {
        return ud->tag_1_ == _XLUA_TAG_1 && ud->tag_2_ == _XLUA_TAG_2;
    }

#if XLUA_ENABLE_LUD_OPTIMIZE
    struct WeakObjData {
        void* obj;
        const TypeDesc* desc;
    };

    struct LightData {
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
            return WeakObjRef{ref_index, ref_serial};
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
    WeakObjData GetWeakObjData(int weak_index, int obj_index);
    void SetWeakObjData(int weak_idnex, int obj_index, void* obj, const TypeDesc* desc);
    int GetMaxWeakIndex();

    inline bool IsUd(LightData ud, const TypeDesc* desc) {
        if (ud.lud_index == 0)
            return false;

        if (ud.lud_index <= GetMaxWeakIndex()) {
            auto data = GetWeakObjData(ud.weak_index, ud.ref_index);
            return data.desc != nullptr && IsBaseOf(desc, data.desc);
        } else {
            const auto* ud_desc = GetTypeDesc(ud.lud_index);
            return ud_desc != nullptr && IsBaseOf(desc, ud_desc);
        }
    }

    inline bool IsUd(LightData ud, ICollection* collection) {
        return false;
    }

    inline void* As(LightData ud, const TypeDesc* desc) {
        if (ud.lud_index == 0)
            return nullptr;

        if (ud.lud_index <= GetMaxWeakIndex()) {
            auto data = GetWeakObjData(ud.weak_index, ud.ref_index);
            if (data.desc == nullptr || !IsBaseOf(desc, data.desc))
                return nullptr;
            if (data.desc->weak_proc.getter(ud.ToWeakRef()) == nullptr)
                return nullptr;
            return _XLUA_TO_SUPER_PTR(data.obj, data.desc, desc);
        } else {
            const auto* ud_desc = GetTypeDesc(ud.lud_index);
            if (ud_desc == nullptr || !IsBaseOf(desc, ud_desc))
                return nullptr;
            return _XLUA_TO_SUPER_PTR(ud.ToObj(), ud_desc, desc);
        }
    }

    inline void* As(LightData ud, ICollection* collection) {
        return nullptr;
    }

    template <typename Ty>
    inline Ty* As(LightData ud) {
        const auto* desc = Support<Ty>::Desc();
        if (ud.lud_index == 0)
            return nullptr;

        if (ud.lud_index <= GetMaxWeakIndex()) {
            auto data = GetWeakObjData(ud.weak_index, ud.ref_index);
            if (data.desc == nullptr || !IsBaseOf(desc, data.desc))
                return nullptr;
            if (data.desc->weak_proc.getter(WeakObjRef{ ud.ref_index, ud.ref_serial }) == nullptr)
                return nullptr;
            return static_cast<Ty*>(_XLUA_TO_SUPER_PTR(data.obj, data.desc, desc));
        } else {
            const auto* ud_desc = GetTypeDesc(ud.lud_index);
            if (ud_desc == nullptr || !IsBaseOf(desc, ud_desc))
                return nullptr;
            return static_cast<Ty*>(_XLUA_TO_SUPER_PTR(ud.ptr, ud_desc, desc));
        }
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    struct UdCache {
        int ref;
        void* ud;
    };

    struct StateData {
        UserData* LoadRawUd(int index) {
            if (lua_type(l_, index) != LUA_TUSERDATA)
                return nullptr;

            auto* ud = static_cast<UserData*>(lua_touserdata(l_, index));
            if (ud->tag_1_ != _XLUA_TAG_1 || ud->tag_2_ != _XLUA_TAG_2)
                return nullptr;
            return ud;
        }

        template <typename Ty>
        inline bool IsUd(int index) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index)) {
                LightData ld;
                ld.value_ = lua_touserdata(l_, index);
                return IsUd(ld, Support<Ty>::Desc());
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadUserData(index);
            if (ud == nullptr)
                return false;
            return IsUd(ud, Support<Ty>::Desc());
        }

        template <typename Ty>
        Ty* LoadUd(int index) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index)) {
                LightData ld;
                ld.value_ = lua_touserdata(l_, index);
                return As<Ty>(ld);
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadUserData(index);
            if (ud == nullptr)
                return nullptr;
            return As<Ty>(ud);
        }

        // value
        template <typename Ty>
        inline void PushUd(const Ty& obj) {
        }

        // collection ptr
        template <typename Ty, typename std::enable_if<std::is_same<CollectionCategory, typename Support<Ty>::category>::value, int>::type = 0>
        inline void PushUd(Ty* ptr) {
        }

        // delcared type ptr
        template <typename Ty, typename std::enable_if<std::is_same<DeclaredCategory, typename Support<Ty>::category>::value, int>::type = 0>
        inline void PushUd(Ty* ptr) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            //TODO:
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            //TODO:
        }

        // smart ptr
        template <typename Sty, typename Ty>
        inline void PushUd(const Sty& s, Ty* ptr, size_t tag) {
        }

        void OnGc(UserData* ud) {
        }

        int RefObj(int index) { return -1; }
        bool LoadRef(int ref) { return false; }
        void AddRef(int ref) {}
        void DecRef(int ref) {}

        lua_State* l_;
        std::vector<std::unordered_map<void*, UdCache>> shared_ptrs_;
        std::array<std::unordered_map<void*, UdCache>, (int8_t)UdType::kCount - 1> raw_ptrs_;
    };

} // internal

XLUA_NAMESPACE_END
