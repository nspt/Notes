# 基本背景

对于草、树、石头等对象，往往需要将同一个对象/网格绘制多次，每次绘制仅 model 变换不同。此时如果采用普通的绘制命令（`glDrawArrays()`和`glDrawElements()`）逐一绘制，会产生大量 Draw Call，从而增加 CPU 端调用和驱动程序处理的开销，导致 CPU 成为瓶颈。

# 实例化（Instancing）绘制

Instancing 就是为了解决绘制大量相同网格时的性能问题，使用带 *Instanced* 后缀的绘制命令 `glDrawArraysInstanced()` 和 `glDrawElementsInstanced()`，允许一次 Draw Call 指定多个实例，由 GPU 在一次绘制调用中处理这些实例，从而减少 Draw Call 数量，减少 CPU 与 GPU 通信的成本，让 GPU 去完成重复性的工作。这两个函数的参数与不带 Instanced 后缀的版本基本一致，只是最后多了一个表示绘制次数的参数。

# 实例数据的存储

在 Vertex Shader 中，除了 `gl_VertexID`，还有一个 `gl_InstanceID`，`gl_InstanceID` 表示一次绘制调用中，当前所绘制的实例的 ID。通过 `gl_InstanceID` 区分当前实例，通常可以将它作为数组下标，从 UBO、SSBO 或其他实例数据存储中取得当前实例的数据。

在 OpenGL 4.2 之后，可以通过 SSBO（*Shader Storage Buffer Object*）存储大规模的数据，不同实例的相关数据可以通过 SSBO 存储，实例化绘制可以通过 `gl_InstanceID` + SSBO 实现。但是在这之前，实例数据的存储或者说传递是通过实例化数组（*Instanced Array*）实现。（**仅靠 UBO 无法支持大规模的实例数据存储，因为 UBO 有存储空间限制，并且上限不高。**）

实例化数组本质上就是顶点属性（Vertex Attribute），但是通过 `glVertexAttribDivisor(index, divisor)` 修改属性的 divisor 后，可修改该顶点属性的“更新间隔”。默认情况下 divisor 为 0，属性每个顶点都更新（即取下一个值），divisor 为 1 时，属性每个实例更新一次，为 2 时，每两个实例更新一次，以此类推。

为了实现实例化绘制，可以将所有实例的 model 变换存储为 mat4 数组，然后作为顶点属性传递给 shader。但是顶点输入属性单条最多只能 4 个分量，即 vec4，因此实际传输 mat4 顶点属性时，必须拆分成 4 个 vec4：

```glsl
// vertex shader
#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 3) in mat4 a_model; // 实际上顶点属性 3、4、5、6 分别对应 mat4 的四列
```

CPU 侧设置实例数组：

```cpp
glm::mat4 modelMatrices[amount];

// vertex buffer object
unsigned int buffer;
glGenBuffers(1, &buffer);
glBindBuffer(GL_ARRAY_BUFFER, buffer);
glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

glBindVertexArray(VAO);
// vertex attributes
std::size_t vec4Size = sizeof(glm::vec4);
glEnableVertexAttribArray(3); 
glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
glEnableVertexAttribArray(4); 
glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
glEnableVertexAttribArray(5); 
glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
glEnableVertexAttribArray(6); 
glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

glVertexAttribDivisor(3, 1);
glVertexAttribDivisor(4, 1);
glVertexAttribDivisor(5, 1);
glVertexAttribDivisor(6, 1);

glBindVertexArray(0);
```