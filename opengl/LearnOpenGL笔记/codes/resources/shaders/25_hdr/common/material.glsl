struct Material {
    sampler2D diffuse_texture;
    bool diffuse_exist;

    sampler2D specular_texture;
    bool specular_exist;
    float shininess;

    bool pure_color;
    vec3 color;

    sampler2D normal_map;
    bool normal_exist;

    sampler2D height_map;
    bool height_exist;
    float height_scale;

    samplerCube reflect_cube_texture;
    bool reflect_cube_exist;

    samplerCube refract_cube_texture;
    float refract_ratio;
    bool refract_cube_exist;
};
uniform Material material;
