#pragma once
#include <vector>
#include <list>
#include <map>
#include <set>

struct lua_State;

namespace xlua {
    struct xlua_index {};

    template <typename Ty, typename Alloc = std::allocator<Ty>>
    struct vector : public std::vector<Ty, Alloc> {
        xlua_index lua_index;
    };

    template<typename Ty>
    struct VectorInterface {
        static int Iter(lua_State* l);

    };

    enum class CollectorType : int8_t {
        Vector,
        Map,
    };

    struct ObjRef {
        CollectorType type;
        int index;
        int serialNum;

        void DoCall(int val, bool b = false) {

        }
    };

    template <typename... Args>
    struct param_num {
        static constexpr size_t value = sizeof...(Args);
    };

    template <>
    struct param_num<> {
        static constexpr size_t value = 0;
    };

    int GetTop(lua_State* l) { return 0;}

    template <typename... Args>
    bool check_param(lua_State* l, int index) {
        return true;
    }

    typedef int (*LuaFunc)(lua_State*);
    struct Element {
        const char* name;
        const LuaFunc func;
    };

    void test_multi_function() {
        auto e1 = Element {"name",
            [](lua_State* l) -> int {
                ObjRef* obj = nullptr;
                int param = GetTop(l);
                // 由多到少
                if (check_param<int, bool>(l, 1)) {
                    obj->DoCall(1, false);
                    return 1; // do_call
                }

                if (check_param<int>(l, 1)) {
                    obj->DoCall(1);
                    return 1; // do call
                }
                // log error
                return 0;
            },
        };

        auto e2 = Element {
            "name2", [](lua_State* l) -> int {
                //TODO:
                return 0;
            },
        };
    }
}

struct Vector3{
    int x;
    int y;
    int z;

    inline Vector3 operator + (const Vector3& r) const;
    inline Vector3 operator - (const Vector3& r) const;
    inline bool operator == (const Vector3& r) const;
    inline Vector3& operator = (const Vector3& r);
    int Dot(const Vector3& r) const;
    Vector3 Cross(const Vector3& r) const;
    void Add(const Vector3& r);
    void Sub(const Vector3& r);
};

enum class UserDataType {
    Alone,

};

enum class LuaObjectType {
    Ptr,
    Value,
    SharedPtr,
    Vector,
};

template <typename Ty>
struct LuaExportor {
    static_assert(true, "check is deleare export to lua");

    static const char* Name() {
        return nullptr;
    }

    static void Push(Ty& val);
    static void Push(Ty&& val);
    static void Push(const Ty& val);

    template <typename Ry> struct Loader;

    template <> struct Loader<Ty&> {
        static Ty& Do();
    }

    template <> struct Loader<const Ty&> {
        static const Ty& Do();
    }

    template <> struct Loader<Ty&> {
        static Ty& Do();
    }
};

struct TypeInfo {};
struct ExternalInfo {};

template <typename Ty>
struct Identity {};

struct RefValue {
    int index;
    int serialNumber;
};

struct WeakObjRefProc {
    typedef RefValue (MakeProc)(void* obj);
    typedef void* (GetProc)(RefValue);

    MakeProc maker;
    GetProc getter;
};

inline const WeakObjRefProc* QueryWeakObjProc(...) { return nullptr; }

template <typename Ty, typename std::enable_if<true, int>::type = 0>
inline const WeakObjRefProc* QueryWeakObjProc(Identity<Ty>) {
    static constexpr WeakObjRefProc proc{
        [](void* obj) -> RefValue {
            return RefValue{0, 0};
        },
        [](RefValue val) -> void* {
            return nullptr;
        };
    };
    return &proc;
}

struct ITypeCaster {
};

struct TypeInfo {
    int index;
    const char* name;
    ITypeCaster* caster;
    WeakObjRefProc weak_proc;
};

enum class Tag : int8_t {
    //Ptr,        // 指针, 弱引用
    //Value,      // obj, 对应数据
	Declared,
    Colletion,	//
    Extend,     // 扩展
};

enum class OtherTag : int8_t {
	Ptr,
	SamrtPtr,
	Value,
};

struct ExtendInfo {
    TypeInfo* type_info;
    size_t hash_id;
    void* ext;
};

struct ExtendVal {
    void* obj;
    void* data;
};

struct UserDataObj {
    virtual ~UserDataObj() { }

    void* obj;
    void* data;
};

struct UdInfo {
    Tag tag;
    union {
        const TypeInfo* type_info;
        const ExtendInfo* extend_info;
    };
    union {
        RefValue ref_value;
        UserDataObj* user_data;
        void* obj;
    };
	
	// shared_ptr tag
};

struct Lud {
    union {
        struct {
            int64_t type_index : 8;
            int64_t ptr:56;
        };
        struct {
            int64_t type_index : 8;
            int64_t index : 24;
            int64_t serial_num : 32;
        };
    };
};

struct Ud {
	struct {
		const int8_t m = 'x';
		const int8_t j = 'z';
		Tag tag;
		OtherTag mask;
	};
    union {
        TypeInfo* type_info;
		CollectionInfo* colletion_info;
        ExtendInfo* extend_info;
    };
    union {
        RefValue ref_value;
        void* obj;  // raw ptr, raw value
    };
};

struct ObjUd : Ud {
	virtual ~ObjUd() {}
};

struct SmartPtrUd : ObjUd {
	size_t tag_id;
	void* data;
};

template <typename Ty>
struct SmartPtrUdImpl : SmartPtrUd {
    SmartPtrUd(const Ty& v) : ptr(v) {}
	virtual ~SmartPtrUd() {}

    Ty ptr;
};

template <typename Ty>
struct UdImpl : ObjUd {
    UdImpl(const Ty& v) : val() {}
	virtual ~UdImpl() {}

    Ty val;
};

typedef int (*IndexFunc)(lua_State* l);
typedef int (*GetterFunc)(lua_State* l);

struct IndexMeta {
    const char* name;
    IndexFunc getter;
    IndexFunc setter;
};

struct MemberMeta {
    const char* name;
    IndexFunc func;
};

struct ExtendMeta {
    IndexMeta* indexer;
    MemberMeta* member;
};

void* GetObjPtr() {
    return nullptr;
}

struct CollectionTraits {
    virtual int Index(lua_State* l) = 0;
    virtual int NewIndex(lua_State* l) = 0;
	virtual int Insert(lua_State* l) = 0;
    virtual int Length(lua_State* l) = 0;
};

void* CreateArrayData(const Ty& val, ArrayMeta* meta) {
	return nullptr;
}


how to build user data?
what's the user data should be?
1. need a base description about the full user data info
	- weak obj
	- collection type
	- extend type
	- shared ptr
	- value or raw ptr
2. need support value type convert
	- dynamic cast(to sub type), ptr/shared_ptr/(need support extend ptr, not necessary)
	- static case(to base type), ptr/shared_ptr/value/(need support extend ptr, not necessary)

table_conv <-> 转换

template <typename Ty>
struct TableConv {
};

template <typename Ty>
struct Support<TableConv<Ty>> {
	using category = value_tag;
	
	typedef TableConv<Ty> type;
	static void Push(State* l, const TableConv<Ty>>& val) {
	}
	static TableConv<Ty> Load(State* l, int index) {
		return TableConv<Ty>(); 
	}
};

template <typename Ty>
struct Support<std::shared_ptr<Ty>> {
	using category = value_tag;
	
	static bool Check(State* l, int index) {
		1. allow nil
		2. check udinfo
	}
	
	static void Push(State* l, const std::shared_ptr<Ty>& ptr) {
		
	}
	
	static std::shared<Ty> Load(State* l) {
		return std::shared_ptr<Ty>();
	}
}

template <typename Ty>
struct Support<std::vector<Ty>> {
	using category = collection_tag;
	static const size_t type_hash_id = type_id(std::vector<Ty>().hash_id();

	struct Meta {
	};
};

char* 
void* 

struct Vec3 {};

template <>
struct Support<Vec3> {
	using category = extend_tag;
	static const size_t type_hash_id = 0;
	
	struct const ExtendDesc* GetDesc() {
		return nullptr;
	};
};

declear traits {
}
声明行为?

SharedPtr data
CollectionVar data

struct nonsupport{};	// compile time check type support
struct value_tag{};		// not support pointer and reference
struct obj_tag{};		// support pointer get, reference?
struct declear_tag {};	// type has declared to export lua
struct extend_tag {};	// runtime export type to lua(used for template export)
struct table_tag {};	// user for and has some interface?
struct collection_tag{};// 容器












