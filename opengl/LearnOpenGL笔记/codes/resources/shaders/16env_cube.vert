#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;

uniform bool has_tex_coord;

out vec3 v_world_pos;
out vec3 v_world_normal;
out vec2 v_tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 world_pos = model * vec4(a_pos, 1.0);
    gl_Position = projection * view * world_pos;
    v_world_pos = world_pos.xyz;
    v_world_normal = mat3(transpose(inverse(model))) * a_normal;
    v_tex_coord = a_tex_coord;
}
