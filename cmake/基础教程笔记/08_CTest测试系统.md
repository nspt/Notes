# CTest 测试系统

> CTest 只是一套基本框架，负责运行指定的命令/程序，然后检查其返回值是否为 0，来判定测试是否通过，实际测试逻辑和代码需要自己实现。

## 添加测试项（`add_test()`）

```cmake
option(BUILD_TESTING "是否生成 CTest 相关配置的 CMake 传统命名变量" ON)
if (BUILD_TESTING)
    enable_testing() # 使能 CTest
    add_subdirectory(Tests) # 传统做法
endif()
```

在 Tests/CMakeLists.txt 中：
```cmake
function(AddTest arg)
    add_test(
        NAME ${arg}
        COMMAND TestTool ${arg}
    )
endfunction()
```

执行 CTest 测试：
```sh
ctest --test-dir build
ctest --test-dir build -R TestName
```
