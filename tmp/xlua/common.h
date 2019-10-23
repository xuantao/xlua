#pragma once
#include <lua.hpp>
#include <type_traits>
#include <limits>
#include <string>

#define XLUA_NAMESPACE_BEGIN    namespace xlua {
#define XLUA_NAMESPACE_END      }
#define _XLUA_INTERAL_NAMESPACE_BEGIN   namespace internal {
#define _XLUA_INTERAL_NAMESPACE_END     }

#define _XLUA_TAG_1  'x'
#define _XLUA_TAG_2  'l'

// predefine
template <typename Ty> struct Support;

class State;

struct ICollection {
    virtual int Index(State* l) = 0;
    virtual int NewIndex(State* l) = 0;
    virtual int Insert(State* l) = 0;
    virtual int Remove(State* l) = 0;
    virtual int Length(State* l) = 0;
};

struct TypeDesc;
struct ExtendDesc;

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
        size_t tag;
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
