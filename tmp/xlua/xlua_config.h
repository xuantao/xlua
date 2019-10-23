#pragma once
#include <stdint.h>

/* ������������ */
#define XLUA_CONTAINER_INCREMENTAL  1024
/* ������������ֽ��� */
#define XLUA_MAX_TYPE_NAME_LENGTH   256
/* �����־��󻺴��ֽ��� */
#define XLUA_MAX_BUFFER_CACHE       1024

/* ֧�ֶ�̳�
 * �����ڶ�̳�ʱ, ����ָ��ת�����ָ��ʱָ����ܻᷢ��ƫ�ƣ�ÿ�η��ʶ���ʱָ����Ҫ����Ӧת����
 * �����ȷԼ�������ڶ�̳�������Թرմ˹������Ż�Ч�ʡ�
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

/* 64λϵͳ����light user data�Ż�
 * ��������ָ��ʹ��LightUserData����FullUserData, ����Lua��GC������Ч��
*/
#ifndef XLUA_ENABLE_LUD_OPTIMIZE
    #define XLUA_ENABLE_LUD_OPTIMIZE 1
#endif

/* ���ϵͳ�Ƿ�֧�� */
#if XLUA_ENABLE_LUD_OPTIMIZE
    #if INTPTR_MAX != INT64_MAX
        #undef XLUA_ENABLE_LUD_OPTIMIZE
        #define XLUA_ENABLE_LUD_OPTIMIZE 0
    #endif
#endif
