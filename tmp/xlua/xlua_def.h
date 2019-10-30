#pragma once
#include <type_traits>
#include <typeinfo>

#define XLUA_NAMESPACE_BEGIN    namespace xlua {
#define XLUA_NAMESPACE_END      }
#define XLUA_NAMESPACE          xlua::

#define _XLUA_TAG_1  'x'
#define _XLUA_TAG_2  'l'

struct lua_State;

XLUA_NAMESPACE_BEGIN

template <typename Ty> struct Support;
struct TypeDesc;
class State;

/* ¼ìË÷ÀàÐÍID */
template <typename Ty>
struct Identity { typedef Ty type; };

struct weak_obj_tag {};

/* weak obj reference */
struct WeakObjRef {
    int index;
    int serial;
};

inline bool operator == (const WeakObjRef& l, const WeakObjRef& r) {
    return l.index == r.index && l.serial == r.serial;
}
inline bool operator != (const WeakObjRef& l, const WeakObjRef& r) {
    return !(l == r);
}

struct WeakObjProc {
    typedef WeakObjRef(*MakeProc)(void* obj);
    typedef void* (*GetProc)(WeakObjRef);

    size_t tag;
    MakeProc maker;
    GetProc getter;
};

class ObjectIndex;
namespace internal {
    /* xlua weak obj reference support */
    WeakObjRef MakeWeakObjRef(void* obj, ObjectIndex& index);
    void* GetWeakObjPtr(WeakObjRef index_);
}

/* lua weak object reference index */
class ObjectIndex {
    friend void FreeObjectIndex(ObjectIndex&);
    friend WeakObjRef internal::MakeWeakObjRef(void*, ObjectIndex&);

public:
    ObjectIndex() = default;
    ~ObjectIndex() { if (index_ > 0) FreeObjectIndex(*this); }

public:
    ObjectIndex(const ObjectIndex&) { }
    ObjectIndex& operator = (const ObjectIndex&) { }

public:
    inline int GetIndex() const { return index_; }

private:
    int index_ = 0;
};

template<typename Ty>
struct IsWeakObj {
private:
    template <typename U> static auto Check(int)->decltype(std::declval<U>().xlua_obj_index_);
    template <typename U> static auto Check(...)->std::false_type;
public:
    static constexpr bool value = std::is_same<decltype(Check<Ty>(0)), ObjectIndex>::value;
};

template <typename Ty>
struct WeakObjTrais {
private:
    template <typename C>
    static C Query(ObjectIndex C::*);
public:
    typedef typename decltype(Query(&Ty::xlua_obj_index_)) class_type;

    static WeakObjRef Make(void* obj) {
        return internal::MakeWeakObjRef(static_cast<class_type*>(obj), static_cast<Ty*>(obj)->xlua_obj_index_);
    }
    static void* Get(WeakObjRef ref) {
        return static_cast<Ty*>(static_cast<class_type*>(internal::GetWeakObjPtr(ref)));
    }
};

XLUA_NAMESPACE_END

/* declare xlua weak obj reference index member */
#define XLUA_DECLARE_OBJ_INDEX          \
    XLUA_NAMESPACE ObjectIndex xlua_obj_index_

/* declare export type to lua */
#define XLUA_DECLARE_CLASS(ClassName)   \
    const XLUA_NAMESPACE TypeDesc* xLuaGetTypeDesc(XLUA_NAMESPACE Identity<ClassName>)

/* default weak obj proc queryer */
inline XLUA_NAMESPACE WeakObjProc xLuaQueryWeakObjProc(...) { return XLUA_NAMESPACE WeakObjProc{0, nullptr, nullptr}; }

/* query xlua weak obj proc */
template <typename Ty, typename std::enable_if<XLUA_NAMESPACE IsWeakObj<Ty>::value, int>::type = 0>
const XLUA_NAMESPACE WeakObjProc xLuaQueryWeakObjProc(XLUA_NAMESPACE Identity<Ty>) {
    return XLUA_NAMESPACE WeakObjProc {
        typeid(XLUA_NAMESPACE weak_obj_tag).hash_code(),
        &XLUA_NAMESPACE WeakObjTrais<Ty>::Make,
        &XLUA_NAMESPACE WeakObjTrais<Ty>::Get
    };
}
