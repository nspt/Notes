#version 330 core

in vec3 v_view_pos;
in vec3 v_view_normal;
in vec2 v_tex_coord;

out vec4 out_color;

struct Material {
    sampler2D diffuse_map;
    sampler2D specular_map;
    float shininess;
}; 
uniform Material material;

struct Light {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 view_pos;
};
uniform Light light;

void main()
{
    vec3 normal = normalize(v_view_normal);
    vec3 to_light = normalize(light.view_pos - v_view_pos);
    vec3 to_camera = normalize(-v_view_pos);
    vec3 reflected_light = reflect(-to_light, normal);

    vec3 diffuse_frag = vec3(texture(material.diffuse_map, v_tex_coord));
    vec3 specular_frag = vec3(texture(material.specular_map, v_tex_coord));

    vec3 ambient  = light.ambient * diffuse_frag;

    float diff = max(dot(normal, to_light), 0.0);
    vec3 diffuse  = light.diffuse * diff * diffuse_frag;

    float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
    vec3 specular = light.specular * spec * specular_frag;

    out_color = vec4(ambient + diffuse + specular, 1.0);
}