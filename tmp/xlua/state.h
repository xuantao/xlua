#pragma once
#include "common.h"
#include "state_data.h"

XLUA_NAMESPACE_BEGIN

struct State;

struct DeclaredDesc;
struct ExtendDesc;

/* market type is not support */
struct NoneCategory {};
/* value type */
struct ValueCategory {};
/* object type, support to push/load with pointer/reference */
struct _ObjectCategory {};
/* type has declared export to lua */
struct DeclaredCategory : _ObjectCategory {};
/* collection type, as vector/list/map... */
struct CollectionCategory : _ObjectCategory {};
/* user defined export type at runtime */
struct ExtendCategory : _ObjectCategory {};

/* get the support type */
template <typename Ty>
struct TypeTrait {
    typedef typename internal::Trait_<typename std::remove_cv<Ty>::type>::type type;
};

class State {
public:
    lua_State* GetState() { return nullptr; }

    UserData* LoadUserData(int index) {
        return nullptr;
    }

    LightData LoadLightData(int index) {
        return LightData();
    }

    internal::StateData state_;
};

XLUA_NAMESPACE_END
