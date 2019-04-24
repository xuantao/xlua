## xlua
### 简介
---
- 导出C++类型、函数、变量
- 提供与Lua交互

### C++导出到Lua
---
#### 导出常量
> 导出全局变量，枚举等
```cpp
/* file: lua_export.cpp */
#include <xlua_export.h>

// 在全局表中添加字段Version, Name
XLUA_EXPORT_CONSTANT_BEGIN(_G)
  XLUA_CONST_VAR_AS(Version, 1)
  XLUA_CONST_VAR_AS(Name, "xlua")
XLUA_EXPORT_CONSTANT_END()
```
---
#### 导出一段Lua脚本
> XLUA_EXPORT_SCRIPT(Script)

```cpp
/* file: lua_export.cpp */
#include <xlua_export.h>

XLUA_EXPORT_SCRIPT(R"V0G0N(
  function SomeExportFunction()
    --TODO:
  end
)V0G0N");
```
当创建lua环境时会执行导出的脚本。  

---
#### 声明导出类  
使用宏（XLUA_DECLARE_CLASS）在定义类的时声明导出到Lua，并在cpp文件中导出函数、变量（支持静态函数与变量）。  
> 声明宏：XLUA_DECLARE_CLASS(ClassName, \[SuperClass\])  
```cpp
#define XLUA_DECLARE_CLASS(ClassName, ...)  \
  typedef xlua::detail::Declare<ClassName,  \
    typename xlua::detail::BaseType<__VA_ARGS__>::type> LuaDeclare;  \
  xlua::xLuaIndex xlua_obj_index_;          \
  static const xlua::detail::TypeInfo* xLuaGetTypeInfo()
```
- 构建导出类型的继承关系，不支持多继承
- 定义成员变量xlua_obj_index_（对像引用索引），用以维护导出对象的生命期
- 定义静态成员函数xLuaGetTypeInfo，以获取导出类型信息

> 导出示例：
```cpp
/* file: obj.h */
#include <xlua_def.h>

namespace test {
  class Obj {
  public:
    XLUA_DECLARE_CLASS(Obj);
  public:
    void Func() {}
    static void StaticFunc() {}
  private:
     int val_;
     static int static_val_;
  }
}
```
> 在cpp文件中声明导出函数、变量
```cpp
/* file: lua_export.cpp */
#include "obj.h"
#include <xlua_export.h>

XLUA_EXPORT_CLASS_BEGIN(test::Obj)
  XLUA_MEMBER_FUNC(&test::Obj::Func)
  XLUA_MEMBER_FUNC(&test::Obj::StaticFunc)
  XLUA_MEMBER_VAR_AS(val, &test::Obj::val_)
  XLUA_MEMBER_VAR_AS(static_val, &test::Obj::static_val_)
XLUA_EXPORT_CLASS_END() 
```
---
#### 导出外部类型
> 在随便头文件中声明导出某一类型
```cpp
/* file lua_export.h */
#include "obj.h"
#include <xlua_def.h>

XLUA_DECLARE_EXTERNAL(test::Obj);
```
> 在cpp文件中声明导出函数、变量
```cpp
/* file: lua_export.cpp */
#include "lua_export.h"
#include <xlua_export.h>

XLUA_EXPORT_EXTERNAL_BEGIN(test::Obj)
  XLUA_MEMBER_FUNC(&test::Obj::Func)
  XLUA_MEMBER_FUNC(&test::Obj::StaticFunc)
  /* 外部类型不能导出受保护的成员 */
  //XLUA_MEMBER_VAR_AS(val, &test::Obj::val_)
  //XLUA_MEMBER_VAR_AS(static_val, &test::Obj::static_val_)
XLUA_EXPORT_EXTERNAL_END()
```
---
#### 导出扩展类型（obj->table)


### 导出细节
#### 扩展导出成员函数
#### 扩展导出成员函数
#### 类型继承关系
#### C++对象类型转换

### API介绍
#### Lua端接口
#### xLuaState接口

### xlua配置
#### 使用LightUserData优化
#### WeakObjPtr扩展

### 部分实现细节

