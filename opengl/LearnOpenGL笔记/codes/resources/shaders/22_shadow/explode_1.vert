#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 3) in mat4 a_model;

out VS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
} vs_out;

layout (std140) uniform CamData {
    mat4 view;
    mat4 projection;
    vec4 pos;
} cam_data;

void main()
{
    vec4 world_pos = a_model * vec4(a_pos, 1.0);
    // 视图空间位置交给 GS 做爆炸；再由 GS 乘 projection
    gl_Position = cam_data.view * world_pos;
    vs_out.v_world_pos = world_pos.xyz;
    vs_out.v_world_normal = mat3(transpose(inverse(a_model))) * a_normal;
    vs_out.v_tex_coord = a_tex_coord;
}
