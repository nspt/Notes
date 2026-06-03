#version 330 core

in vec2 tex_coord;
out vec4 frag_color;

uniform sampler2D sampler0;
uniform sampler2D sampler1;

void main()
{
    frag_color = mix(
        texture(sampler0, tex_coord),
        texture(sampler1, tex_coord),
        0.2
    );
}