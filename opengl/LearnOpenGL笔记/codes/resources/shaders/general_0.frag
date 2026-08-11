#version 330 core

in VS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
} fs_in;

out vec4 out_color;

layout (std140) uniform CamData {
    mat4 view;
    mat4 projection;
    vec4 pos;
} cam_data;

struct Material {
    sampler2D diffuse_texture[4];
    int diffuse_count;
    sampler2D specular_texture[4];
    int specular_count;
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

struct DirectionalLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation; // x: constant, y: linear, z: quadratic
};

struct SpotLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 direction; // xyz: world direction, w: inner cutoff
    vec4 attenuation; // x: constant, y: linear, z: quadratic, w: outer cutoff
};

layout (std140) uniform LightData {
    ivec4 counts; // x: directional, y: point, z: spot
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point_light[8];
} lightData;

vec3 calcDirectionalLights(in vec3 normal, in vec3 to_camera, in vec3 ambient, in vec3 diffuse, in vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.x; ++i) {
        vec3 light_dir = lightData.directional[i].direction.xyz;
        vec3 reflected_light = reflect(light_dir, normal);
        float diff = max(dot(normal, -light_dir), 0.0);
        float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
        result += lightData.directional[i].ambient.xyz * ambient;
        result += lightData.directional[i].diffuse.xyz * diff * diffuse;
        result += lightData.directional[i].specular.xyz * spec * specular;
    }
    return result;
}

vec3 calcPointLights(in vec3 normal, in vec3 to_camera, in vec3 ambient, in vec3 diffuse, in vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.y; ++i) {
        vec3 to_light = lightData.point_light[i].position.xyz - fs_in.v_world_pos;
        float distance = length(to_light);
        to_light = normalize(to_light);
        vec3 reflected_light = reflect(-to_light, normal);
        float diff = max(dot(normal, to_light), 0.0);
        float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
        float attenuation = 1.0 /
                            (lightData.point_light[i].attenuation.x +
                            lightData.point_light[i].attenuation.y * distance + 
                            lightData.point_light[i].attenuation.z * (distance * distance));
        vec3 contrib = lightData.point_light[i].ambient.xyz * ambient
                     + lightData.point_light[i].diffuse.xyz * diff * diffuse
                     + lightData.point_light[i].specular.xyz * spec * specular;
        result += contrib * attenuation;
    }
    return result;
}

vec3 calcSpotLights(in vec3 normal, in vec3 to_camera, in vec3 ambient, in vec3 diffuse, in vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.z; ++i) {
        vec3 to_light = lightData.spot[i].position.xyz - fs_in.v_world_pos;
        float distance = length(to_light);
        to_light = normalize(to_light);
        vec3 reflected_light = reflect(-to_light, normal);
        float diff = max(dot(normal, to_light), 0.0);
        float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
        float attenuation = 1.0 /
                            (lightData.spot[i].attenuation.x +
                            lightData.spot[i].attenuation.y * distance + 
                            lightData.spot[i].attenuation.z * (distance * distance));
        vec3 light_dir = normalize(lightData.spot[i].direction.xyz);
        float inner_cutoff = lightData.spot[i].direction.w;
        float outer_cutoff = lightData.spot[i].attenuation.w;
        float theta = dot(to_light, -light_dir);
        float epsilon = inner_cutoff - outer_cutoff;
        float intensity = epsilon > 0.0
            ? clamp((theta - outer_cutoff) / epsilon, 0.0, 1.0)
            : (theta > outer_cutoff ? 1.0 : 0.0);
        vec3 contrib = lightData.spot[i].ambient.xyz * ambient
                     + lightData.spot[i].diffuse.xyz * diff * diffuse
                     + lightData.spot[i].specular.xyz * spec * specular;
        result += contrib * attenuation * intensity;
    }
    return result;
}

void main()
{
    vec3 normal = normalize(fs_in.v_world_normal);
    vec3 to_camera = normalize(cam_data.pos.xyz - fs_in.v_world_pos);

    if (material.pure_color) {
        out_color = vec4(material.color, 1);
    } else {
        vec4 ambient = vec4(0);
        vec4 diffuse = vec4(0);
        vec4 specular = vec4(0);
        vec4 env_reflect = vec4(0);
        vec4 env_refract = vec4(0);

        if (material.diffuse_count > 0) {
            ambient = texture(material.diffuse_texture[0], fs_in.v_tex_coord);
            diffuse = texture(material.diffuse_texture[0], fs_in.v_tex_coord);
        }

        if (material.specular_count > 0) {
            specular = texture(material.specular_texture[0], fs_in.v_tex_coord);
        }

        if(diffuse.a < 0.01 && specular.a < 0.01)
            discard;

        if (material.reflect_cube_exist) {
            vec3 R = reflect(-to_camera, normal);
            env_reflect = texture(material.reflect_cube_texture, R);
        }

        if (material.refract_cube_exist) {
            vec3 R = refract(-to_camera, normal, material.refract_ratio);
            env_refract = texture(material.refract_cube_texture, R);
        }

        vec3 directional_color = calcDirectionalLights(normal, to_camera, ambient.rgb, diffuse.rgb, specular.rgb);
        vec3 point_color = calcPointLights(normal, to_camera, ambient.rgb, diffuse.rgb, specular.rgb);
        vec3 spot_color = calcSpotLights(normal, to_camera, ambient.rgb, diffuse.rgb, specular.rgb);

        out_color = vec4(directional_color + point_color + spot_color + env_reflect.rgb + env_refract.rgb, diffuse.a);
    }
}
