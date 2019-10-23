#pragma once
#include "common.h"
#include "state.h"

XLUA_NAMESPACE_BEGIN

/* get the support type */
template <typename Ty>
struct TypeTrait {
    typedef typename internal::Trait_<typename std::remove_cv<Ty>::type>::type type;
};

class State {
//    template <typename Ty> friend struct Support;

public:
    lua_State* GetState() { return nullptr; }

    //UserData* LoadUserData(int index) {
    //    return nullptr;
    //}

    //LightData LoadLightData(int index) {
    //    return LightData();
    //}

    internal::StateData state_;
};

XLUA_NAMESPACE_END
