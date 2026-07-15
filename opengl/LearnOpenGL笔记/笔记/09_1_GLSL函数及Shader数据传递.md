# 函数

GLSL 的函数定义与 C/C++ 类似，函数参数可以用 `in`、`out` 修饰，若没有指定则默认为 `in`，`out` 修饰的参数可以作为出参：

```glsl
void CalcDiffuse(
    in vec3 normal,
    in vec3 lightDir,
    out float diff)
{
    diff = max(dot(normal, lightDir), 0.0);
}
```

# Interface Block

Interface Block 用于在 shader 之间传递数据，相比起普通的 `in`、`out` 变量，Interface Block 类似于结构体的语法形式，实现多个输入输出变量的集合，用于在 Shader 阶段之间传递数据。

```glsl
// vertex shader
out VS_OUT
{
    vec2 TexCoords;
} vs_out;

\\ fragment shader
in VS_OUT
{
    vec2 TexCoords;
} fs_in;
```

# Uniform Block

Uniform Block 是用于从 CPU 侧向 shader 传递大量数据而存在的，相比起单个 uniform 变量，Uniform Block 可以将大量数据甚至数组绑定起来。使用 Uniform Block 步骤：

1. Shader 侧声明一个 Uniform Block
2. CPU 侧创建 UBO 对象（*Uniform Buffer Object*）
3. CPU 侧将 Uniform Block 与某个绑定点关联
4. CPU 侧将 UBO 绑定到同一个绑定点（从而与 Shader 连接上）
5. CPU 侧在需要时，更新 UBO 对象的缓存数据

![UBO](images/advanced_glsl_binding_points.png)

```glsl
// shader
layout (std140) uniform ExampleBlock
{
                     // base alignment  // aligned offset
    float value;     // 4               // 0 
    vec3 vector;     // 16              // 16  (offset must be multiple of 16 so 4->16)
    mat4 matrix;     // 16              // 32  (column 0)
                     // 16              // 48  (column 1)
                     // 16              // 64  (column 2)
                     // 16              // 80  (column 3)
    float values[3]; // 16              // 96  (values[0])
                     // 16              // 112 (values[1])
                     // 16              // 128 (values[2])
    bool boolean;    // 4               // 144
    int integer;     // 4               // 148
};
```

Shader 侧声明 `layout (std140)` 是为了使 Uniform Block 的布局方式满足 std140 所定义的标准，具体标准，`float`、`int`、`bool` 都被定义为 4 字节，简写为 N，基本规则如下：

|Type|Layout rule|
|-|-|
|Scalar e.g. int or bool|Each scalar has a base alignment of N.|
|Vector|Either 2N or 4N. This means that a vec3 has a base alignment of 4N.|
|Array of scalars or vectors|Each element has a base alignment equal to that of a vec4.|
|Matrices|Stored as a large array of column vectors, where each of those vectors has a base alignment of vec4.|
|Struct|The base alignment of a structure is equal to the largest base alignment of any of its members, rounded up to a multiple of vec4 alignment.|

```cpp
// 将 Uniform Block 绑定到绑定点 2
unsigned int ub_index = glGetUniformBlockIndex(shader_id, "Lights");   
glUniformBlockBinding(shader_id, ub_index, 2);

// 创建 UBO
unsigned int uboExampleBlock;
glGenBuffers(1, &uboExampleBlock);

// 设置 UBO 数据，假设 struct_cpu 为 CPU 侧对应的结构体
// CPU 侧结构体必须严格满足 std140 对齐要求，否则不能直接通过 memcpy 或 glBufferData 上传！
// 实际开发中通常使用 alignas(16)、glm 的 std140 对齐类型如 glm::vec4 或者手动计算偏移！
glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
glBufferData(GL_UNIFORM_BUFFER, 152, &struct_cpu, GL_STATIC_DRAW);

// 绑定 UBO 到绑定点 2
glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboExampleBlock); 
// 或者
glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboExampleBlock, 0, 152);

// ...
// 需要更新 UBO 数据时，使用 glBufferSubData()（避免使用 glBufferData()，以免触发可能的内存重分配）
glBindBuffer(GL_UNIFORM_BUFFER, uboExampleBlock);
int b = true; // bools in GLSL are represented as 4 bytes, so we store it in an integer
glBufferSubData(GL_UNIFORM_BUFFER, 144, 4, &b);
// 或者完全重置数据
glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(struct_cpu), &struct_cpu);
glBindBuffer(GL_UNIFORM_BUFFER, 0);
```

> 除了 std140 外，现代 OpenGL 还支持 std430。std430 对数组和结构体的填充更紧凑，主要用于 Shader Storage Buffer Object（SSBO）。