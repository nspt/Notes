struct DirectionalLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct SpotLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 direction;     // xyz
    vec4 attenuation;   // xyz: const/linear/quad
    vec4 cutoff;        // x: inner cos, y: outer cos, z: inv_epsilon
};

struct PointLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation;   // xyz: const/linear/quad
};

layout (std140) uniform LightData {
    ivec4 counts; // x: directional, y: point, z: spot
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point[8];
} lightData;
