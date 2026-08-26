#version 330 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_normal;

out vec3 frag_normal;
out vec3 frag_pos;

uniform mat4 modelTrans;
uniform mat4 viewTrans;
uniform mat4 projTrans;

void main()
{
    vec4 vertex_pos = modelTrans * vec4(in_pos, 1.0);
    gl_Position = projTrans * viewTrans * vertex_pos;
    frag_pos = vertex_pos.xyz;
    frag_normal = mat3(transpose(inverse(modelTrans))) * in_normal;
}