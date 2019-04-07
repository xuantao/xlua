#pragma once
#include <stdio.h>
#include <cassert>

/* 64位系统开启LIGHT_USER_DATA
 * 导出对象指针使用LightUserData代替FullUserData, 避免Lua的GC以提升效率
*/
#ifndef XLUA_USE_LIGHT_USER_DATA
    #define XLUA_USE_LIGHT_USER_DATA 1
#endif

/* 检查系统是否支持 */
#if XLUA_USE_LIGHT_USER_DATA
    #if INTPTR_MAX != INT64_MAX
        #define XLUA_USE_LIGHT_USER_DATA 0
    #endif
#endif

/* 配置是否支持弱指针
 * 修改基类定义宏并实例化基础接口
*/
#ifndef XLUA_ENABLE_WEAKOBJ
    #define XLUA_ENABLE_WEAKOBJ 0
#endif

/* 支持弱对象指针必要的一些实现 */
#if !XLUA_ENABLE_WEAKOBJ
    struct WeakObjBase {};
    #define XLUA_WEAK_OBJ_BASE_TYPE WeakObjBase

    template <typename Ty> struct xLuaWeakObjPtr {
        xLuaWeakObjPtr(Ty*) {}
    };

    inline int xLuaAllocWeakObjIndex(XLUA_WEAK_OBJ_BASE_TYPE* val) {/* assert(false); */return 0; }
    inline int xLuaGetWeakObjSerialNum(int index) { /*assert(false); */return 0; }
    inline XLUA_WEAK_OBJ_BASE_TYPE* xLuaGetWeakObjPtr(int index) { /*assert(false); */return nullptr; }
    template <typename Ty>
    inline Ty* xLuaGetPtrByWeakObj(const xLuaWeakObjPtr<Ty>& obj) { return nullptr; }
#else // XLUA_WEAK_OBJ_BASE_TYPE
    #define XLUA_WEAK_OBJ_BASE_TYPE
    template <typename Ty> using xLuaWeakObjPtr;
    int xLuaAllocWeakObjIndex(XLUA_WEAK_OBJ_BASE_TYPE* val);
    int xLuaGetWeakObjSerialNum(int index);
    XLUA_WEAK_OBJ_BASE_TYPE* xLuaGetWeakObjPtr(int index);
    template <typename Ty> Ty* xLuaGetPtrByWeakObj(const xLuaWeakObjPtr<Ty>& obj);
#endif // XLUA_ENABLE_WEAKOBJ

/* 配置日志输出 */
inline void xLuaLogError(const char* err) {
    printf("%s\n", err);
}

/* 容器容量增量 */
#define XLUA_CONTAINER_INCREMENTAL  1024
/* 类型名称最大字节数 */
#define XLUA_MAX_TYPE_NAME_LENGTH   256
/* 输出日志最大缓存字节数 */
#define XLUA_MAX_BUFFER_CACHE       1024

/* 支持多继承
 * 当存在多继承时，子类指针转向基类指针时指针可能会发生偏移，每次访问对象时指针需要做对应转换。
 * 如果明确约定不存在多继承情况可以关闭此功能以优化效率。
*/
#ifndef XLUA_ENABLE_MULTIPLE_INHERITANCE
    #define XLUA_ENABLE_MULTIPLE_INHERITANCE    1
#endif

#if XLUA_ENABLE_MULTIPLE_INHERITANCE
    #define _XLUA_TO_SUPER_PTR(DstInfo, Ptr, SrcInfo)   (SrcInfo == DstInfo ? Ptr : SrcInfo->caster->ToSuper(Ptr, DstInfo))
    #define _XLUA_TO_WEAKOBJ_PTR(DstInfo, Ptr)          DstInfo->caster->ToWeakPtr(Ptr)
#else // XLUA_ENABLE_MULTIPLE_INHERITANCE
    #define _XLUA_TO_SUPER_PTR(DstInfo, Ptr, Info)      Ptr
    #define _XLUA_TO_WEAKOBJ_PTR(DstInfo, Ptr)          Ptr
#endif // XLUA_ENABLE_MULTIPLE_INHERITANCE
