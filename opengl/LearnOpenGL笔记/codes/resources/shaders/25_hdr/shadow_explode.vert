#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 4) in mat4 a_model;

out VS_OUT {
    vec2 v_uv_coord;
} vs_out;

void main()
{
    // 世界空间位置交给 geometry shader 做爆炸位移
    gl_Position = a_model * vec4(a_pos, 1.0);
    vs_out.v_uv_coord = a_tex_coord;
}
