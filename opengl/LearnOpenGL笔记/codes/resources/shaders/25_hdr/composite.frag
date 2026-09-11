#version 330 core

in vec2 v_uv_coord;

out vec4 out_color;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{
    vec3 hdr = texture(scene, v_uv_coord).rgb;
    vec3 bloom = texture(bloomBlur, v_uv_coord).rgb;
    out_color = vec4(hdr + bloom, 1.0);
}
