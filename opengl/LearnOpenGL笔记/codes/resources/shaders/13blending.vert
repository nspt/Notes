#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;

out vec3 v_view_pos;
out vec3 v_view_normal;
out vec2 v_tex_coord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    mat4 model_view = view * model;
    vec4 view_pos = model_view * vec4(a_pos, 1.0);
    gl_Position = projection * view_pos;
    v_view_pos = view_pos.xyz;
    v_view_normal = mat3(transpose(inverse(model_view))) * a_normal;
    v_tex_coord = a_tex_coord;
}