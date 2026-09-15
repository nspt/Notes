#define MAX_DIRECTIONAL_LIGHT 2
#define MAX_POINT_LIGHT 8
#define MAX_SPOT_LIGHT 4

struct DirectionalLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 shadow; // x: min bias, y: slope bias, z: 1.0 / tex_width, w: 1.0 / tex_height
    mat4 transform;
};

struct SpotLight {
    vec4 position;
    vec4 direction;
    vec4 cutoff;        // x: inner cos, y: outer cos, z: inv_epsilon
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation;   // xyz: const/linear/quad
    vec4 shadow; // x: min bias, y: slope bias, z: 1.0 / tex_width, w: 1.0 / tex_height
    mat4 transform;
};

struct PointLight {
    vec4 position;      // xyz: position, w: near_plane
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation;   // xyz: const/linear/quad, w: far_plane
    vec4 shadow; // x: min bias, y: slope bias, z: tex width, w: tex height
};

layout (std140) uniform LightData {
    ivec4 counts; // x: directional, y: point, z: spot
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point[8];
} lightData;

uniform sampler2D directional_shadow_map[MAX_DIRECTIONAL_LIGHT];
uniform sampler2D spot_shadow_map[MAX_SPOT_LIGHT];
uniform samplerCube point_shadow_map[MAX_POINT_LIGHT];
