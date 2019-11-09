#pragma once
#include "xlua_config.h"
#include <type_traits>
#include <typeinfo>

#define XLUA_NAMESPACE_BEGIN    namespace xlua {
#define XLUA_NAMESPACE_END      }
#define XLUA_NAMESPACE          xlua::

#define _XLUA_TAG_1  'x'
#define _XLUA_TAG_2  'l'

struct lua_State;

XLUA_NAMESPACE_BEGIN

/* type identify */
template <typename Ty>
struct Identity { typedef Ty type; };
/* type support */
template <typename Ty>
struct Support;

/* type describtion */
struct TypeDesc;
/* lua state */
class State;

/* market type is not support */
struct not_support_tag {};
/* value type */
struct value_category_tag {};
/* object type, support to push/load with pointer/reference */
struct object_category_tag_ {};
/* type has declared export to lua */
struct declared_category_tag : object_category_tag_ {};
/* collection type, as vector/list/map... */
struct collection_category_tag : object_category_tag_ {};

/* export to lua function */
typedef int(*LuaFunction)(lua_State* l);
/* export to lua var indexer (get/set) */
typedef int(*LuaIndexer)(State* s, void* obj, const TypeDesc* desc);

/* type caster,
 * used for static_cast pointer to base type or dynamic_cast to derived type
*/
struct TypeCaster {
    typedef void* (*Caster)(void* obj);

    Caster to_super;
    Caster to_derived;
};

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

/* weak object reference support
*/
struct WeakObjProc {
    typedef WeakObjRef(*MakeProc)(void* obj);
    typedef void* (*GetProc)(WeakObjRef);

    size_t tag;
    MakeProc maker;
    GetProc getter;
};

/* exoprt declared type desc */
struct TypeDesc {
    int id;
    const char* name;
#if XLUA_ENABLE_LUD_OPTIMIZE
    uint8_t lud_index;          // lightuserdata index
#endif // XLUA_ENABLE_LUD_OPTIMIZE
    int weak_index;             // weak object reference index(for quick index weak obj info)
    const TypeDesc* super;
    const TypeDesc* child;
    const TypeDesc* brother;
    WeakObjProc weak_proc;      // support weakobjref, such as ObjectIndex
    TypeCaster caster;          // type caster, cast to super/derived
};

/* collection interface */
struct ICollection {
    virtual const char* Name() = 0;
    virtual int Index(void* obj, State* s) = 0;
    virtual int NewIndex(void* obj, State* s) = 0;
    virtual int Insert(void* obj, State* s) = 0;
    virtual int Remove(void* obj, State* s) = 0;
    virtual int Iter(void* obj, State* s) = 0;
    virtual int Length(void* obj) = 0;
    virtual void Clear(void* obj) = 0;
};

/* type desc factory */
struct ITypeFactory {
    virtual ~ITypeFactory() { }
    virtual void SetCaster(TypeCaster caster) = 0;
    virtual void SetWeakProc(WeakObjProc proc) = 0;
    virtual void AddMember(bool global, const char* name, LuaFunction func) = 0;
    virtual void AddMember(bool global, const char* name, LuaIndexer getter, LuaIndexer setter) = 0;
    virtual const TypeDesc* Finalize() = 0;
};

/* cast obj type to super type */
inline void* ToSuper(void* obj, const TypeDesc* src, const TypeDesc* dest) {
    while (src != dest) {
        obj = src->caster.to_super(obj);
        src = src->super;
    }
    return obj;
}

/* cast obj type to derived type */
inline void* ToDerived(void* obj, const TypeDesc* src, const TypeDesc* dest) {
    if (src == dest)
        return obj;
    obj = ToDerived(obj, src, dest->super);
    return src->caster.to_derived(obj);
}

class ObjectIndex;
namespace internal {
    /* xlua weak obj reference support */
    WeakObjRef MakeWeakObjRef(void* obj, ObjectIndex& index);
    void* GetWeakObjPtr(WeakObjRef index_);
}

/* xlua weak object reference index */
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

namespace internal {
    template <typename Ty, bool>
    struct PurifyPtrType {
        typedef typename std::remove_cv<Ty>::type type;
    };

    template <typename Ty>
    struct PurifyPtrType<Ty, true> {
        typedef typename std::add_pointer<
            typename std::remove_cv<typename std::remove_pointer<Ty>::type>::type>::type type;
    };

    template <>
    struct PurifyPtrType<const char*, true> {
        typedef const char* type;
    };

    template <>
    struct PurifyPtrType<const volatile char*, true> {
        typedef const char* type;
    };

    template <>
    struct PurifyPtrType<const void*, true> {
        typedef const void* type;
    };

    template <>
    struct PurifyPtrType<const volatile void*, true> {
        typedef const void* type;
    };
} // namepace internal

  /* purify type, supporter use raw type or pointer type */
template <typename Ty>
struct PurifyType {
    typedef typename internal::PurifyPtrType<
        typename std::remove_cv<Ty>::type, std::is_pointer<Ty>::value>::type type;
};

template <typename Ty>
struct PurifyType<Ty&> {
    typedef typename internal::PurifyPtrType<
        typename std::remove_cv<Ty>::type, std::is_pointer<Ty>::value>::type type;
};

/* check the type is supported*/
template <typename Ty>
struct IsSupport {
    static constexpr bool value =
        !std::is_same<not_support_tag, typename Support<typename PurifyType<Ty>::type>::category>::value;
};

/* check the type is declared to export to lua */
template <typename Ty>
struct IsLuaType {
private:
    template <typename U> static auto Check(int)->decltype(xLuaGetTypeDesc(Identity<U>()), std::true_type());
    template <typename U> static auto Check(...)->std::false_type;
public:
    static constexpr bool value = decltype(Check<typename std::decay<Ty>::type>(0))::value;
};

/* traits type is support xlua weak object reference */
template<typename Ty>
struct IsWeakObj {
private:
    template <typename U> static auto Check(int)->decltype(std::declval<U>().xlua_obj_index_);
    template <typename U> static auto Check(...)->std::false_type;
public:
    static constexpr bool value = std::is_same<decltype(Check<Ty>(0)), ObjectIndex>::value;
};

/* xlua weak object reference support tag */
struct weak_obj_tag {};

template <typename Ty>
struct WeakObjTraits {
private:
    /* detect the class dealcre the xlua_obj_index_ member var */
    template <typename C> static C Query(ObjectIndex C::*);
    typedef typename decltype(Query(&Ty::xlua_obj_index_)) class_type;

    static WeakObjRef Make(void* obj) {
        return internal::MakeWeakObjRef(static_cast<class_type*>(obj), static_cast<Ty*>(obj)->xlua_obj_index_);
    }
    static void* Get(WeakObjRef ref) {
        return static_cast<Ty*>(static_cast<class_type*>(internal::GetWeakObjPtr(ref)));
    }

public:
    static WeakObjProc Proc() {
        return WeakObjProc{typeid(weak_obj_tag).hash_code(), &Make, &Get};
    }
};

/* as c++ 14 */
template<size_t...>
struct index_sequence {};

template <size_t N, size_t... Indices>
struct make_index_sequence : make_index_sequence<N - 1, N - 1, Indices...> {};

template<size_t... Indices>
struct make_index_sequence<0, Indices...> {
    typedef index_sequence<Indices...> type;
};

template <size_t N>
using make_index_sequence_t = typename make_index_sequence<N>::type;

/* object wrapper */
template <typename Ty>
struct ObjectWrapper {
    ObjectWrapper(Ty* p) : ptr(p) {}
    ObjectWrapper(Ty& r) : ptr(&r) {}

    inline bool IsValid() const { return ptr != nullptr; }
    inline operator Ty&() const { return *ptr; }

private:
    Ty* ptr = nullptr;
};

template <>
struct ObjectWrapper<void> {
};

namespace internal {
    State* GetState(lua_State* l);
}

XLUA_NAMESPACE_END

/* empty */
inline const XLUA_NAMESPACE TypeDesc* xLuaGetTypeDesc(XLUA_NAMESPACE Identity<void>) {
    return nullptr;
}

/* default weak obj proc queryer */
inline XLUA_NAMESPACE WeakObjProc xLuaQueryWeakObjProc(...) {
    return XLUA_NAMESPACE WeakObjProc{0, nullptr, nullptr};
}

/* query xlua weak obj proc */
template <typename Ty, typename std::enable_if<XLUA_NAMESPACE IsWeakObj<Ty>::value, int>::type = 0>
inline const XLUA_NAMESPACE WeakObjProc xLuaQueryWeakObjProc(XLUA_NAMESPACE Identity<Ty>) {
    return XLUA_NAMESPACE WeakObjTraits<Ty>::Proc();
}

/* declare xlua weak obj reference index member */
#define XLUA_DECLARE_OBJ_INDEX          \
    XLUA_NAMESPACE ObjectIndex xlua_obj_index_

/* declare export type to lua */
#define XLUA_DECLARE_CLASS(ClassName)   \
    const XLUA_NAMESPACE TypeDesc* xLuaGetTypeDesc(XLUA_NAMESPACE Identity<ClassName>)
