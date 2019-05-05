#pragma once
#include "common.h"
#include "serial_alloc.h"
#include <vector>

XLUA_NAMESPACE_BEGIN

namespace detail
{
    struct TypeDesc;

    struct ArrayObj {
        int serial_num_;
        void* obj_;
        const TypeInfo* info_;
    };

    /* 全局数据
     * 管理、维护系统中使用的全局数据
     * TODO: 这块要考虑支持多线程
    */
    class GlobalVar
    {
        friend struct TypeDesc;
        friend class xlua::xLuaIndex;
    private:
        GlobalVar();
        ~GlobalVar();

    public:
        static bool Startup(LogFunc fn);
        static GlobalVar* GetInstance();

        void Purge();

    public:
        xLuaState* Create(const char* export_module);
        xLuaState* Attach(lua_State* l, const char* export_module);
        void Destory(xLuaState* l);

        ITypeDesc* AllocType(TypeCategory category, const char* name, bool has_obj_index, bool is_weak_obj, const TypeInfo* super);
        const TypeInfo* GetTypeInfo(const char* name) const;
        const TypeInfo* GetLUDTypeInfo(int index) const;

        ArrayObj* AllocObjIndex(xLuaIndex& obj_index, void* obj, const TypeInfo* info);
        ArrayObj* GetArrayObj(int index);

    private:
        void FreeObjIndex(int index);
        void AddTypeInfo(TypeInfo*);
        inline void* SerialAlloc(size_t size) { return alloc_.Alloc(size); }

    private:
        LogFunc fn_log_ = nullptr;
        int type_index_gener_ = 0;                  // 类型编号分配器
        int serial_num_gener_ = 0;                  // lua导出对象序列号分配器
        std::vector<ScriptInfo> scripts_;           // 导出的脚本
        std::vector<const ConstInfo*> const_infos_; // 导出的常量
        std::vector<TypeInfo*> types_;              // 导出的类型
        std::vector<TypeInfo*> lud_types_;          // 主要用于lightuserdata优化的类型

        std::vector<int> free_index_;               // lua导出对象空闲槽
        std::vector<ArrayObj> obj_array_;           // lua导出对象数组

        std::vector<xLuaState*> states_;
        SerialAllocator alloc_;
    };
} // namespace detail

XLUA_NAMESPACE_END
