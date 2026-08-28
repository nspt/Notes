#version 330 core

in vec2 v_tex_coord;

out vec4 out_color;

struct Tex {
    int samples; // 1: 普通纹理；>1: 多重采样，对子采样取均值
    sampler2D tex;
    sampler2DMS tex_MS;
};
uniform Tex tex;

uniform float kernel[9];

vec3 fetchColor(vec2 uv)
{
    if (tex.samples <= 1) {
        return texture(tex.tex, uv).rgb;
    }

    ivec2 size = textureSize(tex.tex_MS);
    ivec2 coord = ivec2(uv * vec2(size));
    coord = clamp(coord, ivec2(0), size - ivec2(1));

    vec3 sum = vec3(0.0);
    int n = min(tex.samples, 16);
    for (int s = 0; s < n; ++s) {
        sum += texelFetch(tex.tex_MS, coord, s).rgb;
    }
    return sum / float(n);
}

void main()
{
    vec2 offsets[9] = vec2[](
        vec2(-1.0,  1.0), // top-left
        vec2( 0.0,  1.0), // top-center
        vec2( 1.0,  1.0), // top-right
        vec2(-1.0,  0.0), // center-left
        vec2( 0.0,  0.0), // center-center
        vec2( 1.0,  0.0), // center-right
        vec2(-1.0, -1.0), // bottom-left
        vec2( 0.0, -1.0), // bottom-center
        vec2( 1.0, -1.0)  // bottom-right
    );

    vec2 texelSize = tex.samples <= 1
        ? 1.0 / vec2(textureSize(tex.tex, 0))
        : 1.0 / vec2(textureSize(tex.tex_MS));

    vec3 color = vec3(0.0);
    for (int i = 0; i < 9; ++i) {
        vec2 uv = v_tex_coord + offsets[i] * texelSize;
        color += fetchColor(uv) * kernel[i];
    }

    out_color = vec4(color, 1.0);
}
