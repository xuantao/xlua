#pragma once
#include "common.h"

XLUA_NAMESPACE_BEGIN

struct State;

struct DeclaredDesc;
struct ExtendDesc;

/* market type is not support */
struct NoneCategory {};
/* value type */
struct ValueCategory {};
/* object type, support to push/load with pointer/reference */
struct ObjectCategory {};
/* type has declared export to lua */
struct DeclaredCategory : ObjectCategory {};
/* collection type, as vector/list/map... */
struct CollectionCategory : ObjectCategory {};
/* user defined export type at runtime */
struct ExtendCategory : ObjectCategory {};

namespace internal {
    template <typename Ty>
    struct Trait_ {
        typedef typename std::remove_cv<
            typename std::remove_reference<
            typename std::remove_pointer<Ty>::type>::type>::type type
    };

    template <>
    struct Trait_<char*> {
        typedef char* type;
    };

    template <>
    struct Trait_<const char*> {
        typedef const char* type;
    };

    template <>
    struct Trait_<void*> {
        typedef void* type;
    };

    template <>
    struct Trait_<const void*> {
        typedef const void* type;
    };
}

/* get the support type */
template <typename Ty>
struct TypeTrait {
    typedef typename internal::Trait_<typename std::remove_cv<Ty>::type>::type type;
};

struct ICollection {
    virtual int Index(State* l) = 0;
    virtual int NewIndex(State* l) = 0;
    virtual int Insert(State* l) = 0;
    virtual int Remove(State* l) = 0;
    virtual int Length(State* l) = 0;
};

struct WeakObjRef {
    int32_t index;
    int32_t serial;
};

enum class UserDataType : int8_t {
    None = 0,
    Declared,
    Collection,
    Extend,
};

enum class UserDataCategory : int8_t {
    Ptr,
    SmartPtr,
    Value,
};

struct LightData {
    union {
        // raw ptr
        struct {
            int64_t type_index : 8;
            int64_t ptr : 56;
        };
        // weak obj ref
        struct {
            int64_t type_index : 8;
            int64_t index : 24;
            int64_t serial_num : 32;
        };
        // none
        struct {
            int64_t value_;
        };
    };
};

struct UserData {
    // describe user data info
    struct {
        int8_t tag_1_;
        int8_t tag_2_;
        UserDataType type;
        UserDataCategory category;
    };
    // data information
    union {
        const DeclaredDesc* declared_desc;
        const ExtendDesc* extend_desc;
        ICollection* collection;
    };
    // own data
    union {
        WeakObjRef ref;
        void* obj;
    };
};

namespace internal {
    struct ObjData : UserData {
        virtual ~ObjData() { }
    };

    struct SmartPtrData : ObjData {
        size_t tag_id;
        void* data;
    };

    template <typename Ty>
    struct SmartPtrDataImpl : SmartPtrData {
        SmartPtrDataImpl(const Ty& v) : val(v) { }
        virtual ~SmartPtrDataImpl() { }

        Ty val;
    };

    template <typename Ty>
    struct ValueData : ObjData {
        ValueData(const Ty& v) : val(v) {}
        virtual ~ValueData() { }

        Ty val;
    };
} // namepace internal

XLUA_NAMESPACE_END
