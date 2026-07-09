#version 330 core

in vec2 v_tex_coord;

out vec4 out_color;

uniform float viewport_width;
uniform float viewport_height;

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
    float x_offset = 1.0 / viewport_width;
    float y_offset = 1.0 / viewport_height;
    vec2 offsets[9] = vec2[](
        vec2(-x_offset,  y_offset), // top-left
        vec2( 0.0f,      y_offset), // top-center
        vec2( x_offset,  y_offset), // top-right
        vec2(-x_offset,  0.0f),   // center-left
        vec2( 0.0f,      0.0f),   // center-center
        vec2( x_offset,  0.0f),   // center-right
        vec2(-x_offset, -y_offset), // bottom-left
        vec2( 0.0f,     -y_offset), // bottom-center
        vec2( x_offset, -y_offset)  // bottom-right    
    );

    // avoid optimize
    if (material.specular_count > 0
        || material.shininess > 0
        || material.pure_color
        || material.color[0] == 0) {
        out_color = vec4(1.0);
    }

    vec3 sample_texel[9];
    if (material.diffuse_count > 0) {
        for(int i = 0; i < 9; i++) {
            sample_texel[i] = vec3(texture(material.diffuse_texture[0], v_tex_coord.xy + offsets[i]));
        }
    }
    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; i++) {
        color += sample_texel[i] * kernel[i];
    }
    
    out_color = vec4(color, 1.0);
}