#version 330 core

layout (location = 0) in vec3 in_pos;

uniform mat4 modelTrans;
uniform mat4 viewTrans;
uniform mat4 projTrans;

void main()
{
    gl_Position = projTrans * viewTrans * modelTrans * vec4(in_pos, 1.0);
}