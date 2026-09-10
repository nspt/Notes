struct ShadowBias {
    float min_bias;
    float slope_bias;
};

struct DirLightShadow {
    mat4 transform;
    ShadowBias bias;
    vec2 texel_size;
    sampler2D map;
};

struct OmniLightShadow {
    ShadowBias bias;
    float far_plane;
    vec2 texel_size;
    samplerCube map;
};

struct ShadowMap {
    ivec4 counts; // x: directional, y: point, z: spot
    DirLightShadow directional[2];
    DirLightShadow spot[4];
    OmniLightShadow point[8];
};
uniform ShadowMap shadowMap;
