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

vec3 toneMap(vec3 color)
{
    if (enable_tone_mapping) {
        color = vec3(1.0) - exp(-color * exposure);
    }
    return color;
}

vec3 sampleMapped(vec2 uv)
{
    return toneMap(texture(quad_texture, uv).rgb);
}

vec3 sampleColor()
{
    if (!enable_kernel) {
        return sampleMapped(v_tex_coord);
    }

    // 先 tone map 再卷积：拉普拉斯在 HDR 上卷积后接近 0，再映射会几乎全黑
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
        color += sampleMapped(v_tex_coord + offsets[i] * tex_offset) * kernel[i];
    }
    return color;
}

void main()
{
    vec3 color = sampleColor();
    color = pow(abs(color), vec3(1.0 / gamma));
    out_color = vec4(color, 1.0);
}
