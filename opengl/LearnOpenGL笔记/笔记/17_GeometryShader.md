# Geometry Shader 基础

![图形管线](images/pipeline.png)

Geometry Shader 在图形管线的 Vertex Shader 和 Fragment Shader 之间，是一个可选的 Shader，它的输入和输出都是图元（Primitive）。GS 的意义是让 GPU 能够在顶点处理之后、光栅化之前，以“图元”为单位动态修改、生成或丢弃图元，从而实现几何形状的程序化处理。

GS 的编译、链接和 VS、FS 类似：

```cpp
geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
glShaderSource(geometryShader, 1, &gShaderCode, NULL);
glCompileShader(geometryShader);  
//[...]
glAttachShader(program, geometryShader);
glLinkProgram(program);  
```

在 GS 中，需要指明 GS 的输入和输出类型：

```glsl
layout (points) in; // 输入是点图元
layout (line_strip, max_vertices = 2) out; // 输出是 line_strip 图元，最多 2 个顶点
```

`in` 表示输入的图元类型，`layout()` 中的类型可以是：
- points：绘制 `GL_POINTS` 时使用，输入顶点数为 1
- lines：绘制 `GL_LINES` 或 `GL_LINE_STRIP` 时使用，输入顶点数为 2
- lines_adjacency：绘制 `GL_LINES_ADJACENCY` 或 `GL_LINE_STRIP_ADJACENCY` 时使用，输入顶点数为 4
- triangles：绘制 `GL_TRIANGLES`，`GL_TRIANGLE_STRIP` 或 `GL_TRIANGLE_FAN` 时使用，输入顶点数为3
- triangles_adjacency：绘制 `GL_TRIANGLES_ADJACENCY` 或 `GL_TRIANGLE_STRIP_ADJACENCY` 时使用，输入顶点数为 6

`out` 表示输出的图元类型，`layout()` 中的类型可以是：
- points
- line_strip
- triangle_strip

> strip 表示顶点复用形成连续的图像，比如 3 个顶点形成 line strip，则第 2 个顶点既是第一个线段的终点，也是第二个线段的起点，三角形情况类似\
> ![line_strip](images/geometry_shader_line_strip.png)\
> ![triangle_strip](images/geometry_shader_triangle_strip.png)

> adjancency 会额外提供邻接图元的顶点信息，具体略。

`in` 的顶点数表示组成图元的顶点数量，根据类型不同，顶点数是确定的。

`out` 则是通过 `layout()` 中的 `max_vertices = n` 来表明一次 GS 调用最多能够输出多少个顶点，不是一定输出这么多，也不是说图元会由这么多顶点组成，比如 GS 输出 `triangle_strip` 且 `max_vertices` 为 5 的情况下，GS 可以输出 3 个顶点组成 1 个三角形，也可以输出 4 个顶点组成 2 个三角形，或者 5 个顶点组成 3 个三角形。

因为输入的图元可能由多个顶点组成，所以 GS 需要以数组的形式访问输入图元的顶点，通过内置变量 `gl_in`（一个 Interface Block）：

```glsl
in gl_Vertex
{
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];
```

GS 生成图元的过程大概是：
1. 将需要输出的顶点写入 `gl_Position`（以及此顶点相关 out 变量）
2. 调用 `EmitVertex()` 提交顶点（以及此顶点相关 out 变量）
3. 若组成图元还需要更多顶点，则回到 1，若组成图元的顶点已全部提交，则进入 4
4. 调用 `EndPrimitive()` 提交一个 primitive

两个重要的内置函数：
- `EmitVertex()`：将当前设置好的顶点属性（如 gl_Position）提交为 GS 输出的一个顶点。
- `EndPrimitive()`：结束当前正在生成的 primitive，后续 EmitVertex() 输出的顶点将开始组成新的 primitive。

> 需要注意 `EndPrimitive()` 的调用时间，假设输出 triangle_strip，提交 6 个顶点后调用一次 `EndPritive()`，会生成由 6 个顶点组成的 triangle strip，最终是 4 个邻接三角形；同样 6 个顶点，每提交 3 个顶点后就调用一次 `EndPritive()`，会生成 2 个由 3 个顶点组成的 triangle strip，最终是 2 个三角形。

# GS 示例：点变成多个三角形

假设有 4 个顶点：

```cpp
float points[] = {
	-0.5f,  0.5f, // top-left
	 0.5f,  0.5f, // top-right
	 0.5f, -0.5f, // bottom-right
	-0.5f, -0.5f  // bottom-left
};
```

如果 VS 不做转换，直接将 4 个顶点视为 NDC 输出：

```glsl
#version 330 core
layout (location = 0) in vec2 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}
```

FS 直接输出固定颜色：

```cpp
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);   
}
```

在没有 GS 的情况下，`glDrawArrays(GL_POINTS, 0, 4);` 绘制的输出结果就是 4 个绿色的点：

![points_example](images/geometry_shader_points.png)

假设我们希望 4 个点扩展为由三角形组成的房子图案：

![house](images/geometry_shader_house.png)

可以借助 GS 将输入的 points 变为输出 triangle strip：

```glsl
#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 5) out;

void build_house(vec4 position)
{
    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0);    // 1:bottom-left
    EmitVertex();
    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0);    // 2:bottom-right
    EmitVertex();
    gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0);    // 3:top-left
    EmitVertex();
    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0);    // 4:top-right
    EmitVertex();
    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0);    // 5:top
    EmitVertex();
    EndPrimitive();
}

void main() {    
    build_house(gl_in[0].gl_Position);
}
```

![house](images/geometry_shader_houses.png)

# GS 示例：爆炸效果（三角形沿法线移动）



# GS 示例：显示顶点法线