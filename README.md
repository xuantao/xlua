## xlua
### 简介

- 导出C++类型、函数、变量
- C++与Lua交互

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
#### 导出类型一(内部类)  
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
#### 导出类型二（外部类）
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
#### 导出类型三（obj->table)
用于将一些类型转换为table
> 示例类型
```cpp
/* file: vec2.h */
struct Vec2 {
    Vec2 Cross(const Vec2& v) { return Vec2(); }
    int Dot(const Vec2& v) { return 0; }
    int x;
    int y;
}
```
> 头文件声明
```cpp
/* file: lua_export.h */
#include "vec2.h"
#include <xlua_def.h>

/* 导出到Lua */
void xLuaPush(xlua::xLuaState* l, const Vec2& vec);
/* 从Lua加载 */
Vec2 xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Vec2>);
/* 可选，用于函数调用时参数类型检测，如果没有定义则默认都返回true */
bool xLuaIsType(xlua::xLuaState* l, int i, xlua::Identity<Vec2>);
```
> 实现cpp
```cpp
/* file: lua_export.cpp */
#include "lua_export.h"
#include <xlua_export.h>

XLUA_EXPORT_SCRIPT(R"V0G0N(
  Vec2 = {}
  local vec2Meta = {
    __index = function (t, key)
      return Vec2[key]
    end,
  }
  function Vec2.New(x_, y_)
    return setmettable({x = x_ or 0, y = y_ or 0}, vec2Meta)
  end
  function Vec2.Dot(vec1, vec2)
    --TODO:
  end
  function Vec2.Cross(vec1, vec2)
    --TODO:
  end
)V0G0N");

/* 导出到Lua */
void xLuaPush(xlua::xLuaState* l, const Vec2& vec) {
  l->NewTable();
  l->SetField(-1, "x", vec.x);
  l->SetField(-1, "y", vec.y);
  //TODO: set metatable
}
/* 从Lua加载 */
Vec2 xLuaLoad(xlua::xLuaState* l, int i, xlua::Identity<Vec2>){
  if (l.GetType(i) != LUA_TTABLE)
    return Vec2();
  return Vec2(l->GetField<int>(i, "x"), l->GetField<int>(i, "y"));
}
/* 类型检测 */
bool xLuaIsType(xlua::xLuaState* l, int i, xlua::Identity<Vec2>) {
    return true;
}
```

### 导出细节
#### 扩展类型成员
- 成员函数  
> XLUA_MEMBER_FUNC_EXTEND(Name, Func)  
函数声明格式：  
>> Ry func(Obj* obj, Args...)  
>> int func(Obj* obj, xlua::xLuaState* l)  
>> int func(Obj* obj, lua_State* l)  

- 成员变量
> XLUA_MEMBER_VAR_EXTEND(Name, Get, Set)  
> Get属性声明格式：  
>> Ry Obj_Get_Func(Obj* obj)  
>> int Obj_Get_Func(Obj* obj, xlua::xLuaState* l)  
>> int Obj_Get_Func(Obj* obj, lua_State* l)  
>
> Set属性声明格式：  
>> void Obj_Set_Func(Obj* obj, const Ry& val)  
>> int Obj_Set_Func(Obj* obj, xlua::xLuaState* l)  
>> int Obj_Set_Func(Obj* obj, lua_State* l)  

```cpp
  //TODO: 示例
```
---
#### 类型继承关系
---
#### C++对象类型转换
C++对象导出到lua的类型可以是指针、值对象、共享指针，由此在从Lua加载数据，导出函数调用时需要处理对象的类型关系。  
flowchard

### API介绍
#### Lua端接口
#### xLuaState接口

### xlua配置
#### 使用LightUserData优化
#### WeakObjPtr扩展

### 部分实现细节

