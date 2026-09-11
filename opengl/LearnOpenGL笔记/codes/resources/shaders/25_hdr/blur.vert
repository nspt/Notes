#version 330 core

// 全屏后处理：顶点已是 NDC
layout (location = 0) in vec3 a_pos;
layout (location = 2) in vec2 a_tex_coord;

out vec2 v_uv_coord;

void main()
{
    gl_Position = vec4(a_pos.xy, 0.0, 1.0);
    v_uv_coord = a_tex_coord;
}
