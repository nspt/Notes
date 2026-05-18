# CMake 语言（CMakeLang）基础

## 关键概念/背景：

1. CMakeLang 中所有对象均为字符串，由分号分隔的字符串可以被视为列表
2. `cmake -P` 表示以“脚本模式”运行一个 CMake 文件，此时无需 `project()` 在 CMake 文件中
3. 一般 *.cmake* 文件用于存储 CMake 脚本（包含工具/函数），存于项目的 *cmake* 目录下

设置变量：`set(<var> <value> [PARENT_SCOPE])`

读取变量：`${var}`

## 宏与函数

宏示例：

```cmake
macro(MacroName Arg)
    message(${Arg})      # 输出实参的“名字字符串”，即 MyVar
    message(${${Arg}})   # 这里输出的才是实参的值，即 Old
    set(${Arg} "New")    # 宏直接作用于被调用处的作用域，实参名对应的变量被修改
endmacro()

set(MyVar "Old")
MacroName(MyVar)
message(${MyVar})        # 输出 New
```

函数示例：

```cmake
function(FuncName Arg)
    message(${Arg})      # 输出实参的“名字字符串”，即 MyVar
    message(${${Arg}})   # 这里输出的才是实参的值，即 Old
    set(${Arg} "New")    # 实参名对应的变量，仅在函数作用域被改变
endfunction()

set(MyVar "Old")
FuncName(MyVar)
message(${MyVar})        # 输出 Old
```

宏和函数都能看到被调用处的所有变量，但函数有自己的作用域，要使变量在函数的*父作用域*生效：

1. `set(<var> <name> PARENT_SCOPE)`
2. `return(PROPAGATE <var> [var2...])`

> **注意！！** 宏和函数调用时，传参传递的是变量名（或者说字符串），因此宏/函数内要读取*变量值*时，需要展开！函数返回给父作用域时也要注意使用正确的变量名！

特殊的变量：

1. `ARGV`：宏/函数的所有参数（分号隔开的字符串，列表）
2. `ARGN`：宏/函数的非预期参数（分号隔开的字符串，列表），比如宏/函数预期有 3 个参数，实际上实参有 5 个，则多出的 2 个放在 ARGN

## 条件判断

```cmake
if(Cond1)
    # command
elseif(Cond2)
    # command
else()
    # command
endif()
```

`if()` 中允许直接写变量名而不展开，"TRUE"/"ON"/"YES"（包括小写）均视为真，"0"/"OFF"/"Not Found"等视为假，具体支持的字符串需要参考文档，但是实际使用尽量采用统一风格

> 复合条件按以下优先顺序进行评估：
> 1. 括号
> 2. 一元测试符，如 `COMMAND`，`POLICY`，`TARGET`，`TEST`，`EXISTS`，`IS_READABLE`，`IS_WRITABLE`，`IS_EXECUTABLE`，`IS_DIRECTORY`，`IS_SYMLINK`，`IS_ABSOLUTE`，`DEFINED`
> 3. 二元测试符，如 `EQUAL`，`LESS`，`LESS_EQUAL`，`GREATER`，`GREATER_EQUAL`，`STREQUAL`，`STRLESS`，`STRLESS_EQUAL`，`STRGREATER`，`STRGREATER_EQUAL`，`VERSION_EQUAL`，`VERSION_LESS`，`VERSION_LESS_EQUAL`，`VERSION_GREATER`，`VERSION_GREATER_EQUAL`，`PATH_EQUAL`，`IN_LIST`，`IS_NEWER_THAN`，`MATCHES`
> 4. 一元逻辑运算符 `NOT`
> 5. 二元逻辑运算符 `AND` 和 `OR`，从左到右

## 循环

```cmake
foreach(var IN LISTS list) # 其它形式，如遍历范围、指定 items 见文档
    # command
endforeach()

while (Cond)
    # command
endwhile()
```

## `include()`

`include()` 工作效果类似 C++，将 *.cmake* 文件（或 CMake 自带的一些模块）包含进 CML 就地展开，与 `add_subdirectory()` 的区别是其没有子作用域概念，不会有相对路径自动转换等操作。