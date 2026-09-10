#version 330 core

in vec2 v_tex_coord;

out vec4 out_color;

uniform sampler2D quad_texture;
uniform bool enable_kernel;
uniform float kernel[9];
uniform vec2 tex_offset; // 1/width, 1/height

uniform float exposure;
uniform float gamma;
uniform bool enable_tone_mapping;

vec3 sampleColor()
{
    if (!enable_kernel) {
        return texture(quad_texture, v_tex_coord).rgb;
    }

    vec2 offsets[9] = vec2[](
        vec2(-1.0,  1.0),
        vec2( 0.0,  1.0),
        vec2( 1.0,  1.0),
        vec2(-1.0,  0.0),
        vec2( 0.0,  0.0),
        vec2( 1.0,  0.0),
        vec2(-1.0, -1.0),
        vec2( 0.0, -1.0),
        vec2( 1.0, -1.0)
    );

    vec3 color = vec3(0.0);
    for (int i = 0; i < 9; ++i) {
        color += texture(quad_texture, v_tex_coord + offsets[i] * tex_offset).rgb * kernel[i];
    }
    return color;
}

void main()
{
    vec3 color = sampleColor();
    if (enable_tone_mapping) {
        color = vec3(1.0) - exp(-color * exposure);
    }
    color = pow(color, vec3(1.0 / gamma));
    out_color = vec4(color, 1.0);
}
