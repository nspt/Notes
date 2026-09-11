#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 4) in mat4 a_model;

out VS_OUT {
    vec3 v_ws_pos;
    vec3 v_ws_normal;
    vec2 v_uv_coord;
} vs_out;

#include "common/cam_data.glsl"

void main()
{
    vec4 world_pos = a_model * vec4(a_pos, 1.0);
    gl_Position = cam_data.view * world_pos;
    vs_out.v_ws_pos = world_pos.xyz;
    vs_out.v_ws_normal = mat3(transpose(inverse(a_model))) * a_normal;
    vs_out.v_uv_coord = a_tex_coord;
}
