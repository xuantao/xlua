#pragma once
#include "xlua_config.h"
#include <stdarg.h>
#include <stdio.h>
#include <cstddef>

/* 名字空间 */
#define XLUA_NAMESPACE_BEGIN    namespace xlua {
#define XLUA_NAMESPACE_END      } // namespace xlua

XLUA_NAMESPACE_BEGIN

namespace detail {
    /* 全局数据 */
    class GlobalVar;
    /* 导出类型信息 */
    struct TypeInfo;
}

/* 日志输出回调类型
 * 默认使用printf输出
*/
typedef void(*LogFunc)(const char*);

class xLuaState;

// std::indetity
template <typename Ty>
struct Identity {
    typedef Ty type;
};

/* xlua弱引用编号
 * 管理导出对象生命周期
*/
class xLuaIndex {
    friend class detail::GlobalVar;
public:
    xLuaIndex() = default;
    ~xLuaIndex();

public:
    xLuaIndex(const xLuaIndex&) { }
    xLuaIndex& operator = (const xLuaIndex&) { }

public:
    inline int GetIndex() const { return index_; }

private:
    int index_ = -1;
};

/* 日志缓存 */
class xLuaLogBuffer {
public:
    xLuaLogBuffer(char* buf, size_t capacity) : buf_(buf), capacity_(capacity) {
        buf_[0] = 0;
    }

public:
    inline const char* Data() const { return buf_; }
    inline char* Buf() const { return buf_; }
    inline size_t Size() const { return write_; }
    inline size_t Capacity() const { return capacity_; }

    void Log(const char* fmt, ...);

    inline const char* Finalize() {
        if (write_ && write_ <= capacity_ && buf_[write_-1] == '\n')
            buf_[--write_] = 0; // remove last '\n'
        return buf_;
    }

    inline void Reset() {
        write_ = 0;
        buf_[0] = 0;
    }

private:
    size_t write_ = 0;
    size_t capacity_;
    char* buf_;
};

XLUA_NAMESPACE_END

/* 扩展类型
 * 将执行类型转为lua table等对象
*/
inline bool xLuaIsType(xlua::xLuaState*, xlua::Identity<std::nullptr_t>) { return true; }
inline std::nullptr_t xLuaLoad(xlua::xLuaState*, int, xlua::Identity<std::nullptr_t>) { return nullptr; }
void xLuaPush(xlua::xLuaState*, std::nullptr_t);

/* 声明对象引用索引（引用计数）
 * 管理导出对象生命周期
*/
#define XLUA_DECLARE_OBJ_INDEX          xlua::xLuaIndex xlua_obj_index_

/* 声明导出类 */
#define XLUA_DECLARE_CLASS(ClassName)   const xlua::detail::TypeInfo* xLuaGetTypeInfo(xlua::Identity<ClassName>)
