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
在一个统一的头文件中声明导出外部类型
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
