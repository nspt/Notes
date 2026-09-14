#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 4) in mat4 a_model;

out vec3 v_tex_coord;

#include "common/cam_data.glsl"

void main()
{
    vec4 p = cam_data.projection * mat4(mat3(cam_data.view)) * vec4(a_pos, 1.0);
    gl_Position = p.xyww;
    v_tex_coord = a_pos;
}
