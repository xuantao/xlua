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
[导出相关宏列表](https://github.com/xuantao/xlua/blob/master/doc/MACRO.md)  
包含头文件[<xlua_export.h>](https://github.com/xuantao/xlua/blob/master/xlua/xlua_export.h)  

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
#### C++对象类型转换
C++对象导出到lua的类型可以是指针、值对象、共享指针，由此在从Lua加载数据，导出函数调用时需要处理对象的类型关系。  
- 对象在通过xLuaState::Load, 以及函数调用时的转换关系  
- 自动转换到基类  

![类型转换关系](https://github.com/xuantao/xlua/blob/master/doc/img/ptr_value_convent.png?raw=true)

### API介绍
全局接口，包含头文件[<xlua.h>](https://github.com/xuantao/xlua/blob/master/xlua/xlua.h)

- [bool Startup(LogFunc fn);](#api_Startup)
- [void Shutdown();](#api_Shutdown)
- [xLuaState* Create(const char* export_module);](#api_Create)
- [xLuaState* Attach(lua_State* l, const char* export_module);](#api_Attach)

<span id="api_Startup">bool Startup(LogFunc fn);  
LogFunc：
> typedef void(\*LogFunc)(const char\*);  
  接受xlua的日志输出

启动xlua，初始化内部数据，如果在已经成功启动xlua以后尝试再此启动，会返回false。

<span id="api_Shutdown">void Shutdown();  
结束xlua，会销毁xlua创建的所有xLuaState对象。当销毁以后可以再次通过Startup启动xlua。

<span id="api_Create">xLuaState* Create(const char* export_module);  
创建一个lua状态机，会导出实现的类型、常量和脚本。  
> export_module：用于指定系统导出的全局变量的顶层表名称，可以为空。  

<span id="api_Attach">xLuaState* Attach(lua_State* l, const char* export_module);  
向指定lua状态机挂接xlua系统。
> export_module：用于指定系统导出的全局变量的顶层表名称，可以为空。  

---
### xlua提供常用对象  
包含头文件[<xlua_state.h>](https://github.com/xuantao/xlua/blob/master/xlua/xlua.h)
#### xLuaState
提供与Lua交互，将数据压栈、从栈上获取数据、调用Lua函数等。支持传递的数据类型有基础类型、声明导出的类型、lua表、lua函数等。压栈还额外可以压入函数指针、function对象，但不提供获取原始函数指针、function对象的方法。  
```cpp

```
   

#### xLuaTable
引用lua表，被引用的table不会被GC。  

#### xLuaFunction
引用lua函数，被引用的函数不会被GC。  

#### xLuaGuard
守卫lua栈。

#### xLuaCallGaurd
守卫lua函数调用栈，当对象释放时清除缓存在栈上的返回值。
```cpp
xLuaState* l;
const char* str = nullptr
if (auto guard = l->Call("GetSomeString", std::tie(str))) {
  //TODO:
}
```
> 这里xLuaCallGuard会确保获取的str的值时有效的，避免栈清空的时候lua对象被GC掉。

---
#### Lua端接口
全局名字table：xlua  


- Cast  
> obj xlua.Cast(obj, type_name)  

将对象转换为指定类型的对象，如果成功返回指定类型对象，反之返回nil  
在xlua环境中子类能够自动转换为基类，所以使用此接口尝试将子类转换为基类对象仍会直接返回子类对象，此接口的主要用途为将基类转换为子类对象。  
注意效率，内部实现使用了dynamic_cast校验有效性。  


- IsValid
> boolean xlua.IsValid(obj)  

判定对象有效性，使用类型导出方式一（内部类）或是开启WeakObjPtr，lua中持有的对象可能会变成无效值，使用此接口可以判定对象是否有效。  

- Type
> string xlua.Type(obj)  

获取对象类型

- GetTypeMeta
> table xlua.GetTypeMeta(type_name)  

获取指定类型的元表，可能通过元表对象给指定类型扩展成员函数。
```lua
local meta = xlua.GetTypeMeta("test.Obj")
-- visit meta members,
-- member type is function or light user data
for k, v in pairs(meta) do
  print(k, v)
end
-- add extend member function
meta.LuaExtend = function (obj, v)
  --TODO:
end
```





### xlua配置
包含头文件[<xlua_config.h>](https://github.com/xuantao/xlua/blob/master/xlua/xlua_config.h)  
- #define XLUA_CONTAINER_INCREMENTAL  1024
> 配置数据缓存数组每次增量大小  
缓存数组包含（引用的lua数据，table/function)  
导出的方式一（内部类）的引用索引  

- #define XLUA_MAX_TYPE_NAME_LENGTH   256
> 最大类型名字长度

- #define XLUA_MAX_BUFFER_CACHE       1024
> 最大缓存buff，用来生成输出日志

- #define XLUA_ENABLE_MULTIPLE_INHERITANCE    1
> 是否开启多继承，当类型存在多继承时，子类指针向基类指针转换时需要显示转换。
```cpp
struct A {};
struct B {};
struct D : A, B {};

void test() {
  D d;
  B* b_ptr = &d;
  void* v_ptr = &d;
  assert(v_ptr != d_ptr); // 地址存在偏移
}
```

- #define XLUA_ENABLE_LUD_OPTIMIZE 1
> 开启LightUserData  

在64位系统中，对象地址实际只是用了低48位，高16位空置未被使用，将需要导出的对象指针与对应类型索引打包成LightUserData导出到lua中，可以避免lua的gc提升效率。

- #define XLUA_ENABLE_WEAKOBJ 0
> 开启弱对象指针支持  

弱对像指针是一种对象生命期管理策略，若要开启对应支持需实现相关中转接口。
```cpp
/* 配置基类类型 */
#define XLUA_WEAK_OBJ_BASE_TYPE
/* 配置弱对像指针类型 */
template <typename Ty> using xLuaWeakObjPtr;
/* 获取对象索引编号 */
int xLuaAllocWeakObjIndex(XLUA_WEAK_OBJ_BASE_TYPE* val);
/* 获取对象序列号 */
int xLuaGetWeakObjSerialNum(int index);
/* 获取对象指针 */
XLUA_WEAK_OBJ_BASE_TYPE* xLuaGetWeakObjPtr(int index);
/* 获取对象指针 */
template <typename Ty> Ty* xLuaGetPtrByWeakObj(const xLuaWeakObjPtr<Ty>& obj);
```

### [实现细节](https://github.com/xuantao/xlua/blob/master/xlua/doc/DETAIL.md)

