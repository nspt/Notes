#version 330 core

out vec4 frag_color;

uniform vec3 objectColor;
uniform vec3 lightColor;

void main()
{
    frag_color = vec4(lightColor * objectColor, 1.0);
}