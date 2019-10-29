#pragma once
#include "xlua_config.h"
#include "xlua_def.h"
#include <lua.hpp>
#include <type_traits>
#include <limits>
#include <string>

XLUA_NAMESPACE_BEGIN

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
/* user defined export type at runtime */
struct extend_category_tag : object_category_tag_ {};

/* 导出到Lua类型 */
typedef int(*LuaFunction)(lua_State* l);
typedef int(*LuaIndexer)(State* l, void* obj, const TypeDesc* info);

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

/* 类型转换器
 * 基础类型可以子类->基类，基类->子类
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
    //StringView name;
    const char* name;
    LuaIndexer getter;
    LuaIndexer setter;
};

struct ExportFunc {
    //StringView name;
    const char* name;
    LuaFunction func;
};

enum class TypeCategory {
    kGlobal = 1,
    kClass,
    kTemplate,  // type desc register at runtime
};

/* 类型描述, 用于构建导出类型信息 */
struct ITypeCreator {
    virtual ~ITypeCreator() { }
    virtual void SetCaster(ITypeCaster* caster) = 0;
    virtual void SetWeakProc(WeakObjProc proc) = 0;
    virtual void AddMember(const char* name, LuaFunction func, bool global) = 0;
    virtual void AddMember(const char* name, LuaIndexer getter, LuaIndexer setter, bool global) = 0;
    virtual const TypeDesc* Finalize() = 0;
};

/* exoprt declared type desc */
struct TypeDesc {
    static constexpr uint8_t kObjRefIndex = 0xff;

    int id;
    TypeCategory category;
    //StringView name;
    const char* name;
#if XLUA_ENABLE_LUD_OPTIMIZE
    uint8_t lud_index;              // 用于lightuserdata类型索引
#endif // XLUA_ENABLE_LUD_OPTIMIZE
    WeakObjProc weak_proc;      //
    TypeDesc* super;            // 父类信息
    TypeDesc* child;            // 子类
    TypeDesc* brother;          // 兄弟
    ITypeCaster* caster;        // 类型转换器
    ExportVar* mem_vars;        // 成员变量
    ExportVar* global_vars;     // 全局(静态)变量
    ExportFunc* mem_funcs;      // 成员函数
    ExportFunc* global_funcs;   // 全局(静态)函数
};

/* collection interface */
struct ICollection {
    virtual const char* Name() = 0;
    virtual int Insert(void* obj, State* l) = 0;
    virtual int Remove(void* obj, State* l) = 0;
    virtual int Index(void* obj, State* l) = 0;
    virtual int NewIndex(void* obj, State* l) = 0;
    virtual int Iter(void* obj, State* l) = 0;
    virtual int Length(void* obj) = 0;
    virtual void Clear(void* obj) = 0;
};

namespace internal {
    enum class UdType : int8_t {
        kNone = 0,
        kDeclaredType,
        kCollection,
    };

    enum class UvType : int8_t {
        kNone = 0,
        kPtr,
        kSmartPtr,
        kValue,
    };

    struct FullData {
        // describe user data info
        struct {
            int8_t tag_1_ = _XLUA_TAG_1;
            int8_t tag_2_ = _XLUA_TAG_2;
            UdType dc = UdType::kNone;
            UvType vc = UvType::kNone;
        };
        // data information
        union {
            const TypeDesc* desc;
            ICollection* collection;
        };
        // obj data ptr
        union {
            void* obj;
            WeakObjRef ref;
        };

    public:
        FullData() = default;

        FullData(void* p, ICollection* col) {
            dc = UdType::kCollection;
            vc = UvType::kPtr;
            collection = col;
            obj = p;
        }

        FullData(void* p, const TypeDesc* d) {
            dc = UdType::kDeclaredType;
            vc = UvType::kPtr;
            desc = d;
            obj = p;
        }

    protected:
        FullData(void* p, UvType v, ICollection* col) {
            dc = UdType::kCollection;
            vc = v;
            collection = col;
            obj = p;
        }

        FullData(void* p, UvType v, const TypeDesc* d) {
            dc = UdType::kDeclaredType;
            vc = v;
            desc = d;
            obj = p;
        }
    };

#define ASSERT_FUD(ud) assert(ud && ud->tag_1_ == _XLUA_TAG_1 && ud->tag_2_ == _XLUA_TAG_2)

    struct WeakUd : FullData {
        WeakUd(void* p, const TypeDesc* desc) :
            FullData(p, desc), ref(desc->weak_proc.maker(p)) {}

        WeakObjRef ref;
    };

    struct ObjData : FullData {
        template <typename Ty>
        ObjData(Ty* obj, UvType uv) : FullData(obj, uv, Support<Ty>::Desc()) { }
        virtual ~ObjData() { }
    };

    struct SmartPtrData : ObjData {
        template <typename Ty>
        SmartPtrData(Ty* obj, size_t t, void* d) :
            ObjData(obj, UvType::kSmartPtr), tag(t), data(d) {}
        virtual ~SmartPtrData() {}

        size_t tag;
        void* data;
    };

    template <typename Sy>
    struct SmartPtrDataImpl : SmartPtrData {
        template <typename Ty>
        SmartPtrDataImpl(const Sy& v, Ty* ptr, size_t tag) :
            SmartPtrData(ptr, tag, &val), val(v) { }
        virtual ~SmartPtrDataImpl() { }

        Sy val;
    };

    template <typename Ty>
    struct ValueData : ObjData {
        ValueData(const Ty& v) : ObjData(&val, UvType::kValue), val(v) {}
        virtual ~ValueData() { }

        Ty val;
    };

    struct IAloneUd {
        virtual ~IAloneUd() { }
    };

    template <typename Ty>
    struct AloneUd {
        AloneUd(const Ty& v) : obj(v) {}
        virtual ~AloneUd() {}

        Ty obj;
    };

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

template <typename Ty>
struct IsSupport {
    static constexpr bool value = !std::is_same<not_support_tag,
        typename Support<typename PurifyType<Ty>::type>::type>::value;
};

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
    template <typename U> static auto Check(int)->decltype(::xLuaGetTypeDesc(Identity<U>()), std::true_type());
    template <typename U> static auto Check(...)->std::false_type;
};

namespace internal {
    State* GetState(lua_State* l);
}
XLUA_NAMESPACE_END
