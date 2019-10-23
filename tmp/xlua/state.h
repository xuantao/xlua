#pragma once
#include "common.h"
#include <lua.hpp>

XLUA_NAMESPACE_BEGIN

namespace internal {
#if XLUA_ENABLE_LUD_OPTIMIZE
    struct Env {
        int max_weak_index = 0;
    };

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
                int64_t lud_index : 8;
                int64_t index : 24;
                int64_t serial_num : 32;
            };
            // none
            struct {
                int64_t value_;
            };
        };
    };

    WeakObjData GetWeakObjData(int weak_index, int obj_index);
    void SetWeakObjData(int weak_idnex, int obj_index, void* obj, const TypeDesc* desc);

    template <typename Ty, typename std::enable_if<std::is_same<DeclaredCategory, typename Support<Ty>::category>::value, int>::type = 0>
    inline Ty* As(LightData ud) {
        const auto* desc = Support<Ty>::Desc();
        if (ud.lud_index == 0)
            return nullptr;

        if (ud.lud_index <= UniqueObj<Env>::Instance().max_weak_index) {
            // weak obj ref
        }
        else {
            // user data ptr
        }

        return nullptr;
    }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    inline bool IsBaseOf(const TypeDesc* base, const TypeDesc* drive) {
        while (drive && drive != base)
            drive = drive->super;
        return drive == base;
    }

    template <typename Ty>
    inline Ty* As_(UserData* ud, const TypeDesc* desc) {
        if (ud->dc != UdType::kDeclaredType)
            return nullptr;
        if (!IsBaseOf(desc, ud->desc))
            return nullptr;
        if (ud->desc->weak_index != 0) {
            auto* wud = static_cast<WeakRefData*>(ud);
            if (ud->desc->weak_proc->getter(wud->ref) == nullptr)
                return nullptr;
        }
        return static_cast<Ty*(_XLUA_TO_SUPER_PTR(ud->obj, ud->desc, desc));
    }

    template <typename Ty>
    inline Ty* As_(UserData* ud, ICollection* desc) {
        if (ud->dc != UdType::kCollection || ud->collection != desc)
            return nullptr;
        return static_cast<Ty*>(ud);
    }

    template <typename Ty, typename std::enable_if<std::is_base_of<ObjectCategory_, typename Support<Ty>::category>::value, int>::type = 0>
    inline Ty* As(UserData* ud) {
        return As_(ud, Support<Ty>::Desc());
    }


    struct StateData {
        UserData* LoadUserData(int index) {
            if (lua_type(l_, index) != LUA_TUSERDATA)
                return nullptr;

            auto* ud = static_cast<UserData*>(lua_touserdata(l_, index));
            if (ud->tag_1_ != _XLUA_TAG_1 || ud->tag_2_ != _XLUA_TAG_2)
                return nullptr;
            return ud;
        }

        template <typename Ty>
        inline bool IsUd(int index) {
            //TODO: add light user data logic
            auto* ud = LoadUserData(index);
            if (ud == nullptr)
                return false;

            return IsUd(ud, Support<Ty>::Desc());
        }

        bool IsUd(UserData* ud, const TypeDesc* desc) {
            return true;
        }

        bool IsUd(UserData* ud, const ExtendDesc* desc) {
            return true;
        }

        bool IsUd(UserData* ud, const ICollection* desc) {
            return true;
        }

        template <typename Ty>
        bool PushUd(const Ty& obj) {
            return true;
        }

        template <typename Ty>
        inline bool PushUd(Ty* ptr) {
            return PushUdPtr(ptr, std::is_same<DeclaredCategory, Support<Ty>::category>());
        }

        template <typename Sy, typename Ty>
        bool PushUd(const Sy& s, Ty* ptr, size_t tag) {
            return true;
        }

        template <typename Ty>
        bool IsUd() {
            return true;
        }

        template <typename Ty>
        Ty* LoadUd(int index) {
            auto* ud = LoadUserData(index);
            if (ud == nullptr)
                return nullptr;
            return LoadUdPtr<Ty>(ud, std::is_same<DeclaredCategory, Support<Ty>::category>());
        }

        template <typename Ty>
        bool PushUdPtr(Ty* ptr, std::true_type) {
            //TODO: push ptr, try use light user data
            return true;
        }

        template <typename Ty>
        bool PushUdPtr(Ty* ptr, std::false_type) {
            return true;
        }

        template <typename Ty>
        Ty* LoadUdPtr(UserData* ud, std::true_type) {
            //TODO: load ptr, try use light user data
            return nullptr;
        }

        template <typename Ty>
        Ty* LoadUdPtr(UserData* ud, std::false_type) {
            return nullptr;
        }

        lua_State* l_;
    };

} // internal

XLUA_NAMESPACE_END
