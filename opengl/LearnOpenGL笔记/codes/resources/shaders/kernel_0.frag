#version 330 core

in vec2 v_tex_coord;

out vec4 out_color;

sampler2D tex;

uniform float kernel[9];

void main()
{
    vec2 offsets[9] = vec2[](
        vec2(-1.0,   1.0), // top-left
        vec2( 0.0f,  1.0), // top-center
        vec2( 1.0,   1.0), // top-right
        vec2(-1.0,   0.0f), // center-left
        vec2( 0.0f,  0.0f), // center-center
        vec2( 1.0,   0.0f), // center-right
        vec2(-1.0,  -1.0), // bottom-left
        vec2( 0.0f, -1.0), // bottom-center
        vec2( 1.0,  -1.0)  // bottom-right
    );

    vec3 color = vec3(0.0);
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    for(int i = 0; i < 9; i++) {
        vec2 uv = v_tex_coord.xy + offsets[i] * texelSize;
        color += texture(tex, uv).rgb * kernel[i];
    }

    out_color = vec4(color, 1.0);
}