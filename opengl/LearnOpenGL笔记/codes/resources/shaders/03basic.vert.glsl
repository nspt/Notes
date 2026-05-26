#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec4 my_color;

uniform float extra_red_value;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    my_color = vec4(aColor, 1.0f);
    my_color.x += extra_red_value;
}