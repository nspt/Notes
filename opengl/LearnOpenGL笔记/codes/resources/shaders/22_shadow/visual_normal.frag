#version 330 core

out vec4 out_color;

uniform vec3 normal_color;

void main()
{
    out_color = vec4(normal_color, 1);
}
