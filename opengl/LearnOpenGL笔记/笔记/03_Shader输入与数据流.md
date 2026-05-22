# Shader 代码基本结构

```glsl
#version version_number
in type in_variable_name;
in type in_variable_name;

out type out_variable_name;
  
uniform type uniform_name;
  
void main()
{
  // process input(s) and do some weird graphics stuff
  ...
  // output processed stuff to output variable
  out_variable_name = weird_stuff_we_processed;
}
```

GLSL 支持的基本变量类型有 `int`、`bool`、`uint`、`float`、`double`。

GLSL 支持向量类型，如 `vec2`、`vec3`、`vec4`。其中 `vecN` 表示由 N 个 `float` 分量组成的向量，N 的范围是 2 到 4；也有整数、布尔、无符号整数、双精度版本，如 `ivecN`、`bvecN`、`uvecN`、`dvecN`。

向量类型支持一些特殊的语法：

```glsl
vec2 someVec;
vec4 differentVec = someVec.xyxx;
vec3 anotherVec = differentVec.zyw;
vec4 otherVec = someVec.xxxx + anotherVec.yxzy;

vec2 vect = vec2(0.5, 0.7);
vec4 result = vec4(vect, 0.0, 0.0);
vec4 otherResult = vec4(result.xyz, 1.0);
```

# Shader 的输入与输出

Shader 本身是相对独立的程序，通过输入与输出和图形管线中的其他流程进行数据交互。比较特殊的是 Vertex Shader，其输入来自于 Vertex Attribute，而不是另一个 Shader。

Shader 通过关键字 `in` 和 `out` 定义输入与输出变量，若上游 Shader 定义的输出变量与下游 Shader 定义的输入变量的类型和名称都一样，则该数据从上游 Shader 传递给下游 Shader。此连接在链接 Shader Program 时完成。

对于 Vertex Shader 的输入属性，如果未使用 `layout(location = ...)` 显式指定 location，可以在 Shader Program 链接后，通过 `glGetAttribLocation()` 查询其 location。

> 上游 Shader 与下游 Shader 需要*相邻*，比如若 Shader Program 中仅有 Vertex Shader 和 Fragment Shader，则 Vertex Shader 的输出可以连接到 Fragment Shader，若还有 Geometry Shader，则 Vertex Shader 的输出是给到 Geometry Shader。

# Uniform

*Uniform* 是程序向 Shader Program 提供数据的另一种方法，它和 Vertex Attribute 不同之处在于

1. Uniform 对一个 Shader Program 来说是全局的：同一个 Program 内的各个 Shader 阶段都可以访问它，前提是该 Uniform 在对应 Shader 中被声明并且没有被编译器优化掉。
2. 直到主动修改前，Uniform 的值保持不变

Uniform 在 Shader 中通过 `uniform` 关键字声明，程序通过 `glGetUniformLocation()` 获取其编号，然后通过 `glUniformXXX()` 设置 Uniform 的值。

# 交错顶点属性（*Interleaved Vertex Attribute*）

若每个顶点除了坐标，还有其他属性，并且希望连续存储顶点数据时，可以通过 Interleaved Vertex Buffer 实现，关键点在于 `glVertexAttribPointer()` 的 stride 和 offset 属性要正确：

![Interleaved VBO](images/vertex_attribute_pointer_interleaved.png)

```cpp
float vertices[] = {
    // positions         // colors
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
     0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
};
// ......
// position attribute
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
// color attribute
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
glEnableVertexAttribArray(1);
```

# 片段插值（*Fragment Interpolation*）

渲染图元时，光栅化阶段会根据图元覆盖的屏幕区域生成 fragments。由于 fragments 的数量通常远多于顶点数量，因此 Vertex Shader 输出的顶点属性不会简单地“一对一”传给 Fragment Shader，而是会在光栅化过程中被插值（默认情况下），生成每个 fragment 各自的输入值。

以三角形 ABC 为例，若三个顶点分别带有颜色 A、B、C，三角形内部某个点 P 的颜色可表示为：

P 点颜色 = α * A 点颜色 + β * B 点颜色 + γ * C 点颜色

其中：

α + β + γ = 1

α、β、γ 是 P 相对于三角形 ABC 的重心坐标。几何上可以用面积比理解：

α = 面积(△PBC) / 面积(△ABC)
β = 面积(△PCA) / 面积(△ABC)
γ = 面积(△PAB) / 面积(△ABC)

因此，Fragment shader 中接收到的 `in` 变量，通常已经是根据当前 fragment 位置插值后的结果，而不是某个顶点的原始输出值。

![三个顶点组成的三角形](images/fragment_interpolation.png)