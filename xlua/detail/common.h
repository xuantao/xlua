﻿#pragma once
#include "../xlua_def.h"
#include "traits.h"

XLUA_NAMESPACE_BEGIN

namespace detail {
#if XLUA_USE_LIGHT_USER_DATA
    struct LightUserData {
        union {
            struct {
                void* ptr_;
            };
            struct {
                uint32_t serial_;
                uint32_t index_ : 24;
                uint32_t type_ : 8;
            };
        };

        inline void* Ptr() const {
            return ptr_;
        }

        inline void* RawPtr() const {
            return (void*)(reinterpret_cast<uint64_t>(ptr_) & 0x00ffffffffffffff);
        }

        inline void SetRawPtr(void* p) {
            ptr_ = (void*)((reinterpret_cast<uint64_t>(ptr_) & 0xff000000000000)
                | (reinterpret_cast<uint64_t>(p) & 0x00ffffffffffffff));
        }
    };
#endif // XLUA_USE_LIGHT_USER_DATA

    enum class UserDataCategory {
        kValue,
        kRawPtr,
        kSharedPtr,
        kObjPtr,
    };

    /* user data value */
    struct FullUserData {
        FullUserData(UserDataCategory type, void* obj, const TypeInfo* info)
            : type_(type)
            , info_(info)
            , obj_(obj) {
        }

        FullUserData(UserDataCategory type, int32_t index, int32_t serial, const TypeInfo* info)
            : type_(type)
            , info_(info)
            , index_(index)
            , serial_(serial) {
        }

        virtual ~FullUserData() {}
        virtual void* GetDataPtr() { return nullptr; }

        UserDataCategory type_;
        const TypeInfo* info_;
        union {
            void* obj_;
            struct {
                int32_t index_;
                int32_t serial_;
            };
        };
    };

    /* 日志输出 */
    void LogError(const char* fmt, ...);

    /* 导出到Lua类型 */
    typedef int(*LuaFunction)(lua_State* L);
    typedef void(*LuaIndexer)(xLuaState* L, void* obj, const TypeInfo* info);

    /* 类型转换器
     * 基础类型可以子类->基类，基类->子类
    */
    struct ITypeCaster {
        virtual ~ITypeCaster() {}

        virtual void* ToWeakPtr(void* obj) = 0;
        virtual void* ToSuper(void* obj, const TypeInfo* info) = 0;
        virtual void* ToDerived(void* obj, const TypeInfo* info) = 0;
#if XLUA_USE_LIGHT_USER_DATA
        virtual bool ToDerived(detail::LightUserData* ud) = 0;
#endif // XLUA_USE_LIGHT_USER_DATA
        virtual bool ToDerived(detail::FullUserData* ud) = 0;
    };

    /* 类型描述, 用于构建导出类型信息 */
    struct ITypeDesc {
        virtual ~ITypeDesc() { }
        virtual void SetCaster(ITypeCaster* caster) = 0;
        virtual void AddMember(const char* name, LuaFunction func, bool global) = 0;
        virtual void AddMember(const char* name, LuaIndexer getter, LuaIndexer setter, bool global) = 0;
        virtual const TypeInfo* Finalize() = 0;
    };

    /* 常量字段类型 */
    enum class ConstCategory {
        kNone,
        kInteger,
        kFloat,
        kString,
    };

    /* 常量数据 */
    struct ConstValue {
        ConstCategory category;
        const char* name;

        union {
            int int_val;
            float float_val;
            const char* string_val;
        };
    };

    /* 常量信息 */
    struct ConstInfo {
        const char* name;
        const ConstValue* values;
    };

    struct MemberFunc {
        const char* name;
        LuaFunction func;
    };

    struct MemberVar {
        const char* name;
        LuaIndexer getter;
        LuaIndexer setter;
    };

    /* 类型枚举 */
    enum class TypeCategory {
        kInternal,
        kExternal,
        kGlobal,
    };

    /* 导出类型信息 */
    struct TypeInfo {
        int index;
        TypeCategory category;
        const char* type_name;
        bool is_weak_obj;
        int8_t external_type_index; // 外部类型编号, 用于lightuserdata索引类型
        const TypeInfo* super;      // 父类信息
        ITypeCaster* caster;        // 类型转换器
        MemberVar* vars;            // 成员变量
        MemberFunc* funcs;          // 成员函数
        MemberVar* global_vars;     // 全局(静态)变量
        MemberFunc* global_funcs;   // 全局(静态)函数
    };

    /* 获取最顶层基类信息 */
    inline const TypeInfo* GetRootType(const TypeInfo* info) {
        while (info->super)
            info = info->super;
        return info;
    }

    inline void* GetRootPtr(void* ptr, const TypeInfo* info) {
        const TypeInfo* root = GetRootType(info);
        if (info == root)
            return ptr;
        return _XLUA_TO_SUPER_PTR(root, ptr, info);
    }

    inline bool IsBaseOf(const TypeInfo* base, const TypeInfo* info) {
        while (info && info != base)
            info = info->super;
        return info == base;
    }

    template <typename Ty>
    inline xLuaIndex& GetRootObjIndex(Ty* obj) {
        return static_cast<typename InternalRoot<Ty>::type*>(obj)->xlua_obj_index_;
    }

    template <typename Ty, typename std::enable_if<!IsWeakObjPtr<Ty>::value, int>::type = 0>
    inline Ty* ToDerivedWeakPtr(void*) {
        assert(false);
        return nullptr;
    }

    template <typename Ty, typename std::enable_if<IsWeakObjPtr<Ty>::value, int>::type = 0>
    inline Ty* ToWeakPtrImpl(void* obj) {
        return static_cast<Ty*>((XLUA_WEAK_OBJ_BASE_TYPE*)obj);
    }

    template <typename Ty, typename std::enable_if<!IsWeakObjPtr<Ty>::value, int>::type = 0>
    inline Ty* ToWeakPtrImpl(void*) {
        assert(false);
        return nullptr;
    }
} // namespace detail

XLUA_NAMESPACE_END