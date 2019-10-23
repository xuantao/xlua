#pragma once
#include "xlua_config.h"
#include "xlua_def.h"
#include <lua.hpp>
#include <type_traits>
#include <limits>
#include <string>

XLUA_NAMESPACE_BEGIN

// predefine
template <typename Ty> struct Support;
class State;

/* market type is not support */
struct NoneCategory {};
/* value type */
struct ValueCategory {};
/* object type, support to push/load with pointer/reference */
struct ObjectCategory_ {};
/* type has declared export to lua */
struct DeclaredCategory : ObjectCategory_ {};
/* collection type, as vector/list/map... */
struct CollectionCategory : ObjectCategory_ {};
/* user defined export type at runtime */
struct ExtendCategory : ObjectCategory_ {};


/* ������Lua���� */
typedef int(*LuaFunction)(lua_State* l);
typedef void(*LuaIndexer)(State* l, void* obj, const TypeDesc* info);

/* ����ת����
 * �������Ϳ�������->���࣬����->����
*/
struct ITypeCaster {
    virtual ~ITypeCaster() {}

    virtual void* ToSuper(void* obj, const TypeDesc* info) = 0;
    virtual void* ToDerived(void* obj, const TypeDesc* info) = 0;
    virtual void* ToDerivedStatic(void* obj, const TypeDesc* info) = 0;
};

struct StringView {
    const char* str;
    size_t len;
};

struct ExportVar {
    const char* name;
    LuaIndexer getter;
    LuaIndexer setter;
};

struct ExportFunc {
    const char* name;
    LuaFunction func;
};

enum class TypeCategory {
    kGlobal = 1,
    kClass,
    kTemplate,  // type desc register at runtime
};

/* exoprt type desc */
struct TypeDesc {
    static constexpr uint8_t kObjRefIndex = 0xff;

    int id;
    TypeCategory category;
    StringView name;
    bool has_obj_index;         // ӵ�����ü���
    bool is_weak_obj;           // ֧��������
    uint8_t lud_index;          // ����lightuserdata��������
    uint8_t weak_index;             //
    const WeakObjProc weak_proc;    //
    const TypeDesc* super;          // ������Ϣ
    const TypeDesc* child;          // ����
    const TypeDesc* brother;        // �ֵ�
    ITypeCaster* caster;        // ����ת����
    ExportVar* mem_vars;        // ��Ա����
    ExportVar* global_vars;     // ȫ��(��̬)����
    ExportFunc* mem_funcs;      // ��Ա����
    ExportFunc* global_funcs;   // ȫ��(��̬)����
};

/* collection interface */
struct ICollection {
    virtual int Index(void* obj, State* l) = 0;
    virtual int NewIndex(void* obj, State* l) = 0;
    virtual int Length(void* obj, State* l) = 0;
    virtual int Insert(void* obj, State* l) = 0;
    virtual int Remove(void* obj, State* l) = 0;
    virtual int Clear(void* obj, State* l) = 0;
};

struct ExtendDesc;

namespace internal {
    enum class UdType : int8_t {
        kNone = 0,

        kDeclaredType,
        kCollection,

        kCount,
    };

    enum class UvType : int8_t {
        kNone = 0,
        kPtr,
        kSmartPtr,
        kValue,
    };

    struct UserData {
        // describe user data info
        struct {
            int8_t tag_1_;
            int8_t tag_2_;
            UdType dc;
            UvType vc;
        };
        // data information
        union {
            const TypeDesc* desc;
            ICollection* collection;
        };
        // obj data ptr
        void* obj;
    };

    struct WeakRefData : UserData {
        WeakObjRef ref;
    };

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

template <typename Ty>
struct IsDeclaredType {
    static constexpr bool value = decltype(Check<Ty>(0))::value;
private:
    template <typename U> static auto Check(int)->decltype(xLuaGetTypeDesc(Identity<U>()), std::true_type());
    template <typename U> static auto Check(...)->std::false_type;
};

XLUA_NAMESPACE_END
