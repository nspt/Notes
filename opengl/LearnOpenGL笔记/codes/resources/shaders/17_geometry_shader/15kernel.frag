#version 330 core

in vec2 v_tex_coord;

out vec4 out_color;

struct Material {
    sampler2D diffuse_texture[4];
    int diffuse_count;
    sampler2D specular_texture[4];
    int specular_count;
    float shininess;
    bool pure_color;
    vec3 color;
};
uniform Material material;

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

    // avoid optimize
    if (material.specular_count > 0
        || material.shininess > 0
        || material.pure_color
        || material.color[0] == 0) {
        out_color = vec4(1.0);
    }

    vec3 color = vec3(0.0);
    if (material.diffuse_count > 0) {
        vec2 texelSize = 1.0 / vec2(textureSize(material.diffuse_texture[0], 0));
        for(int i = 0; i < 9; i++) {
            vec2 uv = v_tex_coord.xy + offsets[i] * texelSize;
            color += texture(material.diffuse_texture[0], uv).rgb * kernel[i];
        }
    }
    
    out_color = vec4(color, 1.0);
}