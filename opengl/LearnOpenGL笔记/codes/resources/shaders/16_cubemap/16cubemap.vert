#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 2) in vec2 a_tex_coord;

out vec3 v_tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 p = projection * view * model * vec4(a_pos, 1.0);
    gl_Position = p.xyww;
    v_tex_coord = a_pos;
}