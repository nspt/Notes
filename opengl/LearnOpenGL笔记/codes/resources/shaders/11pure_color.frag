#version 330 core

in vec3 v_view_pos;
in vec3 v_view_normal;
in vec2 v_tex_coord;

out vec4 out_color;

struct Material {
    sampler2D diffuse_texture[4];
    int diffuse_count;
    sampler2D specular_texture[4];
    int specular_count;
    float shininess;
};
uniform Material material;

struct DirectionalLight {
    vec4 direction_view;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight {
    vec4 position;
    vec4 position_view;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation; // x: constant, y: linear, z: quadratic
};

struct SpotLight {
    vec4 position;
    vec4 position_view;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 direction_view; // w: inner cutoff
    vec4 attenuation; // x: constant, y: linear, z: quadratic, w: outter cutoff
};

#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_SPOT_LIGHTS 4
#define MAX_POINT_LIGHTS 8
layout (std140) uniform LightData {
    ivec4 counts; // x: directional, y: point, z: spot
    DirectionalLight directional[MAX_DIRECTIONAL_LIGHTS];
    SpotLight spot[MAX_SPOT_LIGHTS];
    PointLight point_light[MAX_POINT_LIGHTS];
} lightData;

vec4 calcDirectionalLights(in vec3 normal, in vec3 to_camera, in vec4 ambient, in vec4 diffuse, in vec4 specular)
{
    vec4 result = vec4(0.0);
    for (int i = 0; i < lightData.counts.x; ++i) {
        vec3 light_dir = lightData.directional[i].direction_view.xyz;
        vec3 reflected_light = reflect(light_dir, normal);
        float diff = max(dot(normal, -light_dir), 0.0);
        float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
        result += lightData.directional[i].ambient * ambient;
        result += lightData.directional[i].diffuse * diff * diffuse;
        result += lightData.directional[i].specular * spec * specular;
    }
    return result;
}

vec4 calcPointLights(in vec3 normal, in vec3 to_camera, in vec4 ambient, in vec4 diffuse, in vec4 specular)
{
    vec4 result = vec4(0.0);
    for (int i = 0; i < lightData.counts.y; ++i) {
        vec3 to_light = lightData.point_light[i].position_view.xyz - v_view_pos;
        float distance = length(to_light);
        to_light = normalize(to_light);
        vec3 reflected_light = reflect(-to_light, normal);
        float diff = max(dot(normal, to_light), 0.0);
        float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
        float attenuation = 1.0 /
                            (lightData.point_light[i].attenuation.x +
                            lightData.point_light[i].attenuation.y * distance + 
                            lightData.point_light[i].attenuation.z * (distance * distance));
        result += lightData.point_light[i].ambient * ambient;
        result += lightData.point_light[i].diffuse * diff * diffuse;
        result += lightData.point_light[i].specular * spec * specular;
        result *= attenuation;
    }
    return result;
}

vec4 calcSpotLights(in vec3 normal, in vec3 to_camera, in vec4 ambient, in vec4 diffuse, in vec4 specular)
{
    vec4 result = vec4(0.0);
    for (int i = 0; i < lightData.counts.z; ++i) {
        vec3 to_light = lightData.spot[i].position_view.xyz - v_view_pos;
        float distance = length(to_light);
        to_light = normalize(to_light);
        vec3 reflected_light = reflect(-to_light, normal);
        float diff = max(dot(normal, to_light), 0.0);
        float spec = pow(max(dot(to_camera, reflected_light), 0.0), material.shininess);
        float attenuation = 1.0 /
                            (lightData.spot[i].attenuation.x +
                            lightData.spot[i].attenuation.y * distance + 
                            lightData.spot[i].attenuation.z * (distance * distance));
        vec3 light_dir = normalize(lightData.spot[i].direction_view.xyz);
        float inner_cutoff = lightData.spot[i].direction_view.w;
        float outer_cutoff = lightData.spot[i].attenuation.w;
        float theta = dot(to_light, -light_dir);
        float epsilon = inner_cutoff - outer_cutoff;
        float intensity = epsilon > 0.0
            ? clamp((theta - outer_cutoff) / epsilon, 0.0, 1.0)
            : (theta > outer_cutoff ? 1.0 : 0.0);
        result += lightData.spot[i].ambient * ambient;
        result += lightData.spot[i].diffuse * diff * diffuse;
        result += lightData.spot[i].specular * spec * specular;
        result *= attenuation * intensity;
    }
    return result;
}

float LinearizeDepth(float depth) 
{
    float near = 0.1;
    float far  = 100.0;
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main()
{
    vec3 normal = normalize(v_view_normal);
    vec3 to_camera = normalize(-v_view_pos);

    vec4 ambient = vec4(0);
    vec4 diffuse = vec4(0);
    vec4 specular = vec4(0);

    if (material.diffuse_count > 0) {
        ambient = texture(material.diffuse_texture[0], v_tex_coord);
        diffuse = texture(material.diffuse_texture[0], v_tex_coord);
    }

    if (material.specular_count > 0) {
        specular = texture(material.specular_texture[0], v_tex_coord);
    }

    vec4 directional_color = calcDirectionalLights(normal, to_camera, ambient, diffuse, specular);
    vec4 point_color = calcPointLights(normal, to_camera, ambient, diffuse, specular);
    vec4 spot_color = calcSpotLights(normal, to_camera, ambient, diffuse, specular);

    //out_color = directional_color + point_color + spot_color;
    out_color = vec4(vec3(LinearizeDepth(gl_FragCoord.z)), 1.0);
}