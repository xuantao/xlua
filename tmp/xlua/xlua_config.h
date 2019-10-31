#pragma once
#include <stdint.h>

/* 容器容量增量 */
#define XLUA_CONTAINER_INCREMENTAL  1024
/* 类型名称最大字节数 */
#define XLUA_MAX_TYPE_NAME_LENGTH   256
/* 输出日志最大缓存字节数 */
#define XLUA_MAX_BUFFER_CACHE       1024

/* 支持多继承
 * 当存在多继承时, 子类指针转向基类指针时指针可能会发生偏移，每次访问对象时指针需要做对应转换。
 * 如果明确约定不存在多继承情况可以关闭此功能以优化效率。
*/
#ifndef XLUA_ENABLE_MULTIPLE_INHERITANCE
    #define XLUA_ENABLE_MULTIPLE_INHERITANCE    1
#endif

#if XLUA_ENABLE_MULTIPLE_INHERITANCE
    #define _XLUA_TO_SUPER_PTR(Ptr, SrcInfo, DstInfo)   (SrcInfo == DstInfo ? Ptr : SrcInfo->caster.to_super(Ptr, SrcInfo, DstInfo))
    #define _XLUA_TO_TOP_SUPER_PTR(Ptr, SrcInfo)        _XLUA_TO_SUPER_PTR(Ptr, SrcInfo, nullptr)
    #define _XLUA_TO_WEAKOBJ_PTR(DstInfo, Ptr)          DstInfo->caster->ToWeakPtr(Ptr)
#else // XLUA_ENABLE_MULTIPLE_INHERITANCE
    #define _XLUA_TO_SUPER_PTR(DstInfo, Ptr, Info)      Ptr
    #define _XLUA_TO_TOP_SUPER_PTR(Ptr, SrcInfo)        Ptr
    #define _XLUA_TO_WEAKOBJ_PTR(DstInfo, Ptr)          Ptr
#endif // XLUA_ENABLE_MULTIPLE_INHERITANCE

/* 64位系统开启light user data优化
 * 导出对象指针使用LightUserData代替FullUserData, 避免Lua的GC以提升效率
*/
#ifndef XLUA_ENABLE_LUD_OPTIMIZE
    ///* 检查系统是否支持 */
    #if INTPTR_MAX == INT64_MAX
        #define XLUA_ENABLE_LUD_OPTIMIZE 1
    #endif
#elif XLUA_ENABLE_LUD_OPTIMIZE
    #define XLUA_MAX_WEAKOBJ_TYPE_COUNT 0
    #if INTPTR_MAX != INT64_MAX
        //TODO: setup compile error
    #endif
#endif
