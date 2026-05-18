# 深入 Target 命令

> Target 命令，即操作 Target 属性的相关命令。

## 常用的 Target 命令

1. `target_sources()`

    指定在构建目标及其依赖项时使用的源文件

2. `target_link_libraries()`

    指定在链接给定目标及其依赖项时要使用的库或标志。

2. `target_compile_definitions()`

    指定目标编译时使用的编译定义：
    ```cmake
    if(OPTION_IN_CMAKE)
        target_compile_definitions(MyLib
            PRIVATE
                MACRO_IN_CXX
        )
    endif()
    ```

3. `target_compile_features()`

    指定编译目标时所需的编译器功能，如 C++ 标准要求：
    ```cmake
    target_compile_features(MyLib PUBLIC cxx_std_20)
    ```

## 进阶 Target 命令

1. `set_target_properties(<targets> ... PROPERTIES <prop1> <value1> [<prop2> <value2>] ...)`

2. `get_target_property(<variable> <target> <property>)`

3. `target_compile_options()` / `target_link_options`

    设置目标的编译与链接选项，一般用于设置平台相关的编译/链接参数：
    ```cmake
    # target_compile_options(MyLib PRIVATE -Wall -Werror)
    # target_link_options(MyLib PRIVATE -T LinksScript.ld)
    if(
        (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") OR
        (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    )

        target_compile_options(Tutorial PRIVATE /W3)
    elseif(
        (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
        (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    )
        target_compile_options(Tutorial PRIVATE -Wall)
    endif()
    ```

4. `target_precompile_headers()`

    添加要预编译的头文件列表，略。

## 少用/谨慎的 Target 命令

1. `target_include_directories()`

    指定目标的包含目录，相当于编译器 `-I` 指令

2. `target_link_directories()`

    指定目标的链接目录，相当于编译器 `-L` 指令

上述指令仅在使用第三方提供的、已编译好、无 CMake 支持的库时使用。