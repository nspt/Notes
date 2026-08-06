#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 3) in mat4 a_model;

out vec3 v_tex_coord;

layout (std140) uniform Matrices {
    mat4 view;
    mat4 projection;
} matrices;

void main()
{
    vec4 p = matrices.projection * matrices.view * a_model * vec4(a_pos, 1.0);
    gl_Position = p.xyww;
    v_tex_coord = a_pos;
}