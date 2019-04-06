#pragma once
#include "xlua_config.h"
#include <lua.hpp>
#include <type_traits>
#include <cstdint>
#include <cassert>

/* 名字空间 */
#define XLUA_NAMESPACE_BEGIN    namespace xlua {
#define XLUA_NAMESPACE_END      } // namespace xlua

XLUA_NAMESPACE_BEGIN

namespace detail {
    /* 获取基类信息(宏变参) */
    template <typename... Tys> struct BaseType { static_assert(sizeof...(Tys) > 1, "not allow multy inherit"); };
    template <> struct BaseType<> { typedef void type; };
    template <typename Ty> struct BaseType<Ty> { typedef Ty type; };

    /* 导出类型复杂关系定义 */
    template <typename Ty, typename By>
    struct Declare {
        typedef By  super;
        typedef Ty  self;
    };

    /* 全局数据 */
    class GlobalVar;
    /* 导出类型信息 */
    struct TypeInfo;
}

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

XLUA_NAMESPACE_END

/* 声明导出Lua类, 在类中插入xlua相关信息
 * 可变宏参数用于设置基类类型(不支持多基类)
 * 1. 关联父、子类关系
 * 2. 添加导出引用编号成员变量
 * 3. 添加获取类型信息的静态成员函数
*/
#define XLUA_DECLARE_CLASS(ClassName, ...)                              \
    typedef xlua::detail::Declare<ClassName,                            \
        typename xlua::detail::BaseType<__VA_ARGS__>::type> LuaDeclare; \
    xlua::xLuaIndex xlua_obj_index_;                                    \
    static const xlua::detail::TypeInfo* xLuaGetTypeInfo()

/* 声明导出外部类 */
#define XLUA_DECLARE_EXTERNAL(ClassName)                                \
    const xlua::detail::TypeInfo* xLuaGetTypeInfo(xlua::Identity<ClassName>)
