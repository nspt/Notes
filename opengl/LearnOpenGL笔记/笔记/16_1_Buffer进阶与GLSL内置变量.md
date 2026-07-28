# Buffer

在 OpenGL 中，一个 Buffer 从本质上来说是一个管理特定 GPU 内存的对象，仅此而已。当我们将缓冲区绑定到特定的缓冲区目标时，我们才赋予它意义。只有当我们将缓冲区绑定到 `GL_ARRAY_BUFFER` 时，它才是一个顶点数组缓冲区，但我们同样可以将其绑定到 `GL_ELEMENT_ARRAY_BUFFER`。OpenGL 内部会为每个目标存储缓冲区的引用，并根据目标不同，对缓冲区进行不同的处理。

## glBufferSubData()

`glBufferData()` 会一次性完成两件事：分配内存 + 写入数据。而如果我们只需要更新/写入数据到已分配内存的 Buffer，那么 `glBufferSubData()` 会更合适：

```cpp
glBufferSubData(GL_ARRAY_BUFFER, 24, sizeof(data), &data); // Range: [24, 24 + sizeof(data)]
```

## glCopyBufferSubData()

可以通过 `glCopyBufferSubData()` 将 Buffer 数据拷贝到另一个 Buffer：

```cpp
void glCopyBufferSubData(GLenum readtarget, GLenum writetarget, GLintptr readoffset,
                         GLintptr writeoffset, GLsizeiptr size);
```

如果源和目标都是同一类 Buffer（比如都是顶点数组缓存），可以借助两个特殊的绑定目标： `GL_COPY_READ_BUFFER` 和 `GL_COPY_WRITE_BUFFER`：

```cpp
glBindBuffer(GL_COPY_READ_BUFFER, vbo1);
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 8 * sizeof(float));

// 或者只让其中一个使用特殊目标
glBindBuffer(GL_ARRAY_BUFFER, vbo1);
glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 8 * sizeof(float));
```

# GLSL 内置变量

## Vertex Shader

### gl_PointSize

`gl_PointSize` 是一个输出型 float 变量，用于表示绘制图元 `GL_POINTS` 时点的像素大小，需要使能 `GL_PROGRAM_POINT_SIZE` 才能在 VS 中使用/写入：

```cpp
glEnable(GL_PROGRAM_POINT_SIZE);
```

```glsl
//shader
void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);    
    gl_PointSize = gl_Position.z;    
}
```

![point_size](images/advanced_glsl_pointsize.png)

### gl_VertexID

`gl_VertexID` 是一个输入型 int 变量（用于读），它表示当前顶点在本次绘制命令中的索引编号，其值由 OpenGL 自动提供，无需作为顶点属性传入。需要注意一点：对于 `glDrawElements()`，`gl_VertexID` 不是 0、1、2……这样的顺序编号，而是索引缓冲区读取到的顶点索引值（再加上 `baseVertex`，如果使用了带 `BaseVertex` 的绘制命令）。

## Fragment Shader

### gl_FragCoord

`gl_FragCoord` 是输入型 vec4 变量，其 x 和 y 表示当前片段的屏幕空间坐标，假设 `glViewport()` 设置窗口尺寸为 800x600，那么 x 就在 [0, 800]，y 在 [0, 600]；z 分量表示当前片段的深度值；w 分量等于裁剪空间坐标的 1/w。

### gl_FrontFacing

`gl_FrontFacing` 是输入型 bool 变量，其表示当前片段是否属于正面三角形，若开启了面剔除（且剔除的是背面），则 `gl_FrontFacing` 总是 true。

### gl_FragDepth

`gl_FragDepth` 是输出型 float 变量，用来修改当前片段的深度值，有效范围是 [0, 1]。当 FS 会写入 `gl_FragDepth` 时，early-z 就会自动关闭，但是自 OpenGL 4.2 起，如果在 FS 中添加以下声明，则 OpenGL 仍然可以完成部分 early-z 测试：

```glsl
layout (depth_<condition>) out float gl_FragDepth;
```

其中 `condition` 可以是以下值：

- any：默认值，early-z 会被关闭
- greater：FS 只会使 `gl_FragCoord.z` 变得更大
- less：FS 只会使 `gl_FragCoord.z` 变得更小
- unchanged：FS 如果写入 `gl_FragCoord.z`，只会写入其原本的值（即不修改值）

```glsl
#version 420 core // note the GLSL version!
out vec4 FragColor;
layout (depth_greater) out float gl_FragDepth;

void main()
{             
    FragColor = vec4(1.0);
    gl_FragDepth = gl_FragCoord.z + 0.1;
}
```