# 配置项与 Cache

> 配置项，表示用于控制 CMake 行为的变量，如 `CMAKE_BUILD_TYPE`、`CMAKE_CXX_STANDARD` 或自定义变量，一般为 Cache 变量。

## Cache 变量

由 `cmake -D` 定义，或在 CML 中由 `option(<variable> "<help_text>" [value])` 定义的变量，也可通过 `set(` 定义（语法见文档，有新旧两种）。

Cache 变量具有粘性，*初始化*后其值会存在构建目录的 *CMakeCache.txt* 中，再次执行 `cmake -B` 不会改变其值，除非再次通过 `-D` 修改。

## CMake 标准变量

由 `CMAKE_` 开头的变量，供 CMake 使用，常见的比如 `CMAKE_CXX_STANDARD`，表示 C++ 标准。

## CMakePresets.json/CMakeUserPresets.json

通过 `-D` 定义配置项过于繁琐，且不好与他人共享，因此可将一套配置预定义好，存于 *CMakePresets.json* 中，以便重复使用，例：

```json
{
  "version": 4,
  "configurePresets": [
    {
      "name": "example-preset",
      "binaryDir": "${sourceDir}/build"
      "cacheVariables": {
        "EXAMPLE_FOO": "Bar",
        "EXAMPLE_QUX": "Baz"
      }
    }
  ]
}
```
