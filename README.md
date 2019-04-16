### xlua
#### 简介
- xlua主要用于C++与Lua脚本间的交互。
1. 方便导出C++类型、函数、变量
2. 方便C++端调用lua函数、获取lua变量


#### 基本用法，导出类型
- 类型方式导出一  
在类中声明导出到Lua
```cpp
/* file: obj.h */

#include <xlua_def.h>

namespace test {
  class Obj {
  public:
    XLUA_DECLARE_CLASS(Obj);
  public:
    void Member() {}
  private:
     int val_;
  }
}


/* file: lua_export.cpp */
#include "obj.h"
#include <xlua_export.h>

XLUA_EXPORT_CLASS_BEGIN(test::Obj)
  XLUA_MEMBER_FUNC(Member)
  XLUA_MEMBER_VAR_AS(val, val_)
XLUA_EXPORT_CLASS_END() 
```
- 类型导出方式二  
在一个统一的文件中声明、实现导出外部类型
```cpp
/* file: obj.h */
namespace test {
  class Obj {
  public:
    void Member() {}
    int val;
  }
}

/* file: lua_export.h */
#include "obj.h"
#include <xlua_def.h>

XLUA_DECLARE_EXTERNAL(test::Obj);

/* file: lua_export.cpp */
#include "lua_export.h"
#include <xlua_export.h>

XLUA_EXPORT_EXTERNAL_BEGIN(test::Obj)
  XLUA_MEMBER_FUNC(Member)
  XLUA_MEMBER_VAR(val)
XLUA_EXPORT_EXTERNAL_END()
```
这两种导出类型的主要区别：  
源码声明导出：
1. 能够导出protected, private成员（变量、函数）。
2. 增加对象保护功能。当导出的对象被释放以后，lua中的对象会被标记无效，避免访问非法指针。
  
外部类行式导出：
1. 无需更改源代码。
2. 导出对象的生命期由使用者保证。
3. 不能访问受保护成员。

##### C++对象到 Lua
当类型被声明导出以后，便可以通过xlua::xLuaState对象将对应类型的对象、指针、共享指针传递到Lua中。
```cpp
xlua::xLuaState* l;
Obj  obj;
Obj* obj_ptr1;
std::shared_ptr<Obj> obj_ptr2;
// 压栈
l->Push(obj);
l->Push(obj_ptr1);
l->Push(obj_ptr2);
// 读取栈上数据
obj = l->Load<Obj>(-3);
obj_ptr1 = l->Load<Obj*>(-2);
obj_ptr2 = l->Load<std::shared_ptr<Obj>>(-1);
```
对象在交互传递过程中的类型转换关系  
1. 子类->父类
2. 指针<->值类型
3. 共享指针->指针
4. 共享指针->值类型
5. 共享指针->共享指针  

#### 注意：
由于共享指针、值数据->指针时，涉及到对象生命期管理问题，需确保使用对应指针lua栈未被清空，以免照成未定义行为。
