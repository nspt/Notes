#version 330 core

in vec2 v_tex_coord;

struct Material {
    sampler2D diffuse_texture;
    bool diffuse_exist;
    sampler2D specular_texture;
    bool specular_exist;
    samplerCube reflect_cube_texture;
    bool reflect_cube_exist;
    samplerCube refract_cube_texture;
    float refract_ratio;
    bool refract_cube_exist;
    float shininess;
    bool pure_color;
    vec3 color;
};
uniform Material material;

void main()
{
    if (!material.pure_color && material.diffuse_exist) {
        vec4 diffuse = texture(material.diffuse_texture, v_tex_coord);
        if (diffuse.a < 0.01)
            discard;
    }
}
