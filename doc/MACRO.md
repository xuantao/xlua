## xlua
### 宏列表
#### 声明宏
包含头文件 <xlua_def.h>

> XLUA_DECLARE_CLASS(ClassName, ...)  

声明导出类
> XLUA_DECLARE_EXTERNAL(ClassName)  

导出外部类

#### 导出宏
包含头文件 <xlua_export.h>

1. 导出脚本
> XLUA_EXPORT_SCRIPT(Str)  

2. 导出常量
> XLUA_EXPORT_CONSTANT_BEGIN(Table)  
XLUA_CONST_VAR(Var)  
XLUA_CONST_VAR_AS(Name, Var)  
XLUA_EXPORT_CONSTANT_END()  

3. 导出全局函数、变量
> XLUA_EXPORT_GLOBAL_BEGIN(Table)  
XLUA_GLOBAL_FUNC(Func)  
XLUA_GLOBAL_FUNC_AS(Name, Func)  
XLUA_GLOBAL_VAR(Var)  
XLUA_GLOBAL_VAR_AS(Name, Var)  
XLUA_GLOBAL_VAR_WARP(Name, GetVar, SetVar)  
XLUA_EXPORT_GLOBAL_END  

4. 导出类型
> XLUA_EXPORT_CLASS_BEGIN(ClassName) or  
XLUA_EXPORT_EXTERNAL_BEGIN(ClassName, ...)  
XLUA_MEMBER_FUNC(Func)  
XLUA_MEMBER_FUNC_AS(Name, Func, ...)  
XLUA_MEMBER_FUNC_EXTEND(Name, Func)  
XLUA_MEMBER_VAR(Var)  
XLUA_MEMBER_VAR_AS(Name, Var)  
XLUA_MEMBER_VAR_WRAP(Name, Get, Set)  
XLUA_MEMBER_VAR_EXTEND(Name, Get, Set)  
XLUA_GLOBAL_FUNC(Func)  
XLUA_GLOBAL_FUNC_AS(Name, Func) 
XLUA_GLOBAL_VAR(Var)  
XLUA_GLOBAL_VAR_AS(Name, Var)  
XLUA_GLOBAL_VAR_WARP(Name, Get, Set)  
XLUA_EXPORT_CLASS_END() or .. 
XLUA_EXPORT_EXTERNAL_END()  

当导出成员是静态的、全局的函数、变量，会对应创建创建table用以访问对应的导出数据。
