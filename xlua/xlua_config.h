#pragma once
#include <stdint.h>

/* when contain is full, will incremental size */
#define XLUA_CONTAINER_INCREMENTAL  4096

/* switch the multiple inheritance optimize
 * if enable this optimize then
 * 1. will directily cast the derived pointer to base pointer
 * 2. not support multiple inheritance, if has multiple inheritance will cause unknown error
*/
#ifndef XLUA_ENABLE_MULTIPLE_INHERITANCE_OPTIMIZE
    #define XLUA_ENABLE_MULTIPLE_INHERITANCE_OPTIMIZE   0
#endif

#if XLUA_ENABLE_MULTIPLE_INHERITANCE_OPTIMIZE
    #define _XLUA_TO_SUPER_PTR(DstInfo, Ptr, Info)      Ptr
#else
    #define _XLUA_TO_SUPER_PTR(Ptr, SrcInfo, DstInfo)   xlua::ToSuper(Ptr, SrcInfo, DstInfo)
#endif

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
