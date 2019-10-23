#pragma once
#include "common.h"
#include <lua.hpp>

XLUA_NAMESPACE_BEGIN

namespace internal {

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
