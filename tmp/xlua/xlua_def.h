#pragma once
#include <type_traits>
#include <typeinfo>

#define XLUA_NAMESPACE_BEGIN    namespace xlua {
#define XLUA_NAMESPACE_END      }
#define XLUA_NAMESPACE          xlua::

//#define _XLUA_INTERAL_NAMESPACE_BEGIN   namespace internal {
//#define _XLUA_INTERAL_NAMESPACE_END     }

#define _XLUA_TAG_1  'x'
#define _XLUA_TAG_2  'l'

struct lua_State;

XLUA_NAMESPACE_BEGIN

struct TypeDesc;
struct State;

/* ¼ìË÷ÀàÐÍID */
template <typename Ty>
struct Identity { typedef Ty type; };

/* lua weak object reference index */
class ObjectIndex {
    friend void FreeObjectIndex(ObjectIndex&);

public:
    ObjectIndex() = default;
    ~ObjectIndex() { FreeObjectIndex(*this); }

public:
    ObjectIndex(const ObjectIndex&) { }
    ObjectIndex& operator = (const ObjectIndex&) { }

public:
    inline int GetIndex() const { return index_; }

private:
    int index_ = -1;
};

template<typename Ty>
struct IsWeakObj {
    static constexpr bool value = std::is_same<decltype(Check<Ty>(0)), ObjectIndex>::value;
private:
    template <typename U> static auto Check(int)->decltype(std::declval<U>().xlua_obj_index_);
    template <typename U> static auto Check(...)->std::false_type;
};

struct WeakObjTag {};

/* weak obj reference */
struct WeakObjRef {
    int index;
    int serial;
};

struct WeakObjProc {
    typedef WeakObjRef (*MakeProc)(void* obj);
    typedef void* (*GetProc)(WeakObjRef);

    size_t tag;
    MakeProc maker;
    GetProc getter;
};

namespace internal {
    /* xlua weak obj reference support */
    WeakObjRef MakeWeakObjRef(void* obj, ObjectIndex& index);
    void* GetWeakObjPtr(WeakObjRef ref);
}

XLUA_NAMESPACE_END

/* declare xlua weak obj reference index member */
#define XLUA_DECLARE_OBJ_INDEX          \
    XLUA_NAMESPACE ObjectIndex xlua_obj_index_

/* declare export type to lua */
#define XLUA_DECLARE_CLASS(ClassName)   \
    const XLUA_NAMESPACE TypeDesc* xLuaGetTypeDesc(XLUA_NAMESPACE Identity<ClassName>)

/* default weak obj proc queryer */
inline const XLUA_NAMESPACE WeakObjProc* xLuaQueryWeakObjProc(...) { return nullptr; }

/* query xlua weak obj proc */
template <typename Ty, typename std::enable_if<XLUA_NAMESPACE IsWeakObj<Ty>::value, int>::type = 0>
const XLUA_NAMESPACE WeakObjProc* xLuaQueryWeakObjProc(XLUA_NAMESPACE Identity<Ty>) {
    static XLUA_NAMESPACE WeakObjProc proc {
        typeid(XLUA_NAMESPACE WeakObjTag).hash_code(),
        [](void* obj) -> WeakObjRef {
            return XLUA_NAMESPACE internal::MakeWeakObjRef(obj, static_cast<Ty*>(obj)->xlua_obj_index_);
        },
        [](WeakObjRef ref) -> void* {
            return XLUA_NAMESPACE internal::GetWeakObjPtr(ref);
        }
    };
    return &proc;
}
