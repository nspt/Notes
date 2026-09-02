#version 330 core

in GS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
    vec3 v_world_tangent;
    vec3 v_view_vec;
    vec3 v_to_point_light[8];
    vec3 v_to_spot_light[4];
    vec4 v_directional_light_space_pos[2];
    vec4 v_spot_light_space_pos[4];
} fs_in;

out vec4 out_color;

layout (std140) uniform CamData {
    mat4 view;
    mat4 projection;
    vec4 pos;
} cam_data;

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
    sampler2D normal_map;
    bool normal_exist;
    sampler2D height_map;
    bool height_exist;
    float height_scale;
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
    mat4 light_space_transform;
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
    mat4 light_space_transform;
};

layout (std140) uniform LightData {
    ivec4 counts; // x: directional, y: point, z: spot
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point_light[8];
} lightData;

struct ShadowMap {
    ivec4 counts; // x: directional, y: point, z: spot
    vec2 shadow_map_texel_size;
    float spot_inv_epsilon[4];
    float directional_min_bias[2];
    float directional_slope_bias[2];
    float spot_min_bias[4];
    float spot_slope_bias[4];
    float point_min_bias[8];
    float point_slope_bias[8];
    float point_far[8];
    sampler2D directional[2];
    sampler2D spot[4];
    samplerCube point_light[8];
};
uniform ShadowMap shadowMap;

mat3 getTBNMatrix(vec3 tangent, vec3 normal)
{
    vec3 T = normalize(tangent);
    vec3 N = normalize(normal);
    // re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    // then retrieve perpendicular vector B with the cross product of T and N
    vec3 B = cross(N, T);

    return mat3(T, B, N);
}

vec3 getViewDirTangent(mat3 tbn, vec3 view_vec)
{
    return normalize(vec3(
        dot(tbn[0], view_vec),
        dot(tbn[1], view_vec),
        dot(tbn[2], view_vec)
    ));
}

vec2 parallaxOcclusionMapping(vec2 texCoords, vec3 viewDir)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy * material.height_scale / viewDir.z;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(material.height_map, currentTexCoords).r;

    for (float i = 0.0; i < numLayers; i += 1.0) {
        if (currentLayerDepth >= currentDepthMapValue)
            break;
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(material.height_map, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.height_map, prevTexCoords).r - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth + 1e-6);
    return mix(currentTexCoords, prevTexCoords, weight);
}

vec3 getFragNormalInWorld(mat3 tbn, vec2 texCoord)
{
    vec3 tangent_normal = texture(material.normal_map, texCoord).xyz * 2.0 - 1.0;
    return normalize(tbn * tangent_normal);
}

float shadowCalculation(vec4 light_space_frag_pos, sampler2D depth_map, float bias)
{
    vec3 coords = light_space_frag_pos.xyz / light_space_frag_pos.w;
    coords = coords * 0.5 + 0.5;

    if (coords.z > 1.0)
        return 0.0;

    float cur_depth = coords.z;
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest_depth = texture(
                depth_map,
                coords.xy + vec2(x, y) * shadowMap.shadow_map_texel_size
            ).r;
            shadow += cur_depth - bias > closest_depth ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

float pointShadowCalculation(vec3 to_light_vec, samplerCube depth_map, float bias, float far_plane, float view_distance)
{
    vec3 light_to_frag = -to_light_vec;
    float cur_depth = length(light_to_frag);
    if (cur_depth > far_plane)
        return 0.0;

    vec3 sample_offsets[20] = vec3[](
        vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
        vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
        vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
        vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
        vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );
    float disk_radius = (1.0 + (view_distance / far_plane)) / 25.0;

    float lit = 0.0;
    for (int i = 0; i < 20; ++i) {
        float closest_depth = texture(depth_map, light_to_frag + sample_offsets[i] * disk_radius).r;
        closest_depth *= far_plane;
        lit += cur_depth - bias > closest_depth ? 0.0 : 1.0;
    }
    return lit / 20.0;
}

vec3 calcDirectionalLights(in vec3 normal, in vec3 to_camera, in vec3 ambient, in vec3 diffuse, in vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.x; ++i) {
        vec3 light_dir = lightData.directional[i].direction.xyz;
        float bias = dot(normal, light_dir) * shadowMap.directional_slope_bias[i]
                   + shadowMap.directional_min_bias[i];
        float shadow = (i < shadowMap.counts.x)
            ? shadowCalculation(
                fs_in.v_directional_light_space_pos[i],
                shadowMap.directional[i],
                bias)
            : 1.0;
        float diff = max(dot(normal, -light_dir), 0.0);
        vec3 halfway = normalize(-light_dir + to_camera);
        float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
        result += lightData.directional[i].ambient.xyz * ambient;
        result += lightData.directional[i].diffuse.xyz * diff * diffuse * shadow;
        result += lightData.directional[i].specular.xyz * spec * specular * shadow;
    }
    return result;
}

vec3 calcPointLights(in vec3 normal, in vec3 to_camera, in float view_distance, in vec3 ambient, in vec3 diffuse, in vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.y; ++i) {
        vec3 to_light_vec = fs_in.v_to_point_light[i];
        float distance = length(to_light_vec);
        vec3 to_light = to_light_vec / distance;
        vec3 light_dir = -to_light;
        float bias = dot(normal, light_dir) * shadowMap.point_slope_bias[i]
                   + shadowMap.point_min_bias[i];
        float diff = max(dot(normal, to_light), 0.0);
        vec3 halfway = normalize(to_light + to_camera);
        float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
        float attenuation = 1.0 /
                            (lightData.point_light[i].attenuation.x +
                            lightData.point_light[i].attenuation.y * distance +
                            lightData.point_light[i].attenuation.z * (distance * distance));
        float shadow = (i < shadowMap.counts.y)
            ? pointShadowCalculation(
                to_light_vec,
                shadowMap.point_light[i],
                bias,
                shadowMap.point_far[i],
                view_distance)
            : 1.0;
        vec3 contrib = lightData.point_light[i].ambient.xyz * ambient
                     + lightData.point_light[i].diffuse.xyz * diff * diffuse * shadow
                     + lightData.point_light[i].specular.xyz * spec * specular * shadow;
        result += contrib * attenuation;
    }
    return result;
}

vec3 calcSpotLights(in vec3 normal, in vec3 to_camera, in vec3 ambient, in vec3 diffuse, in vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.z; ++i) {
        vec3 to_light_vec = fs_in.v_to_spot_light[i];
        float distance = length(to_light_vec);
        vec3 to_light = to_light_vec / distance;
        vec3 light_dir = lightData.spot[i].direction.xyz;
        float bias = dot(normal, light_dir) * shadowMap.spot_slope_bias[i]
                   + shadowMap.spot_min_bias[i];
        float diff = max(dot(normal, to_light), 0.0);
        vec3 halfway = normalize(to_light + to_camera);
        float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
        float attenuation = 1.0 /
                            (lightData.spot[i].attenuation.x +
                            lightData.spot[i].attenuation.y * distance +
                            lightData.spot[i].attenuation.z * (distance * distance));
        float outer_cutoff = lightData.spot[i].attenuation.w;
        float theta = dot(to_light, -light_dir);
        float inv_epsilon = shadowMap.spot_inv_epsilon[i];
        float intensity = inv_epsilon > 0.0
            ? clamp((theta - outer_cutoff) * inv_epsilon, 0.0, 1.0)
            : (theta > outer_cutoff ? 1.0 : 0.0);
        float shadow = (i < shadowMap.counts.z)
            ? shadowCalculation(
                fs_in.v_spot_light_space_pos[i],
                shadowMap.spot[i],
                bias)
            : 1.0;
        vec3 contrib = lightData.spot[i].ambient.xyz * ambient
                     + lightData.spot[i].diffuse.xyz * diff * diffuse * shadow
                     + lightData.spot[i].specular.xyz * spec * specular * shadow;
        result += contrib * attenuation * intensity;
    }
    return result;
}

void main()
{
    vec3 normal = normalize(fs_in.v_world_normal);
    vec2 texCoord = fs_in.v_tex_coord;
    vec3 to_camera = normalize(fs_in.v_view_vec);
    float view_distance = length(fs_in.v_view_vec);

    if (material.height_exist || material.normal_exist) {
        mat3 tbn = getTBNMatrix(fs_in.v_world_tangent, fs_in.v_world_normal);

        if (material.height_exist) {
            vec3 view_dir_tangent = getViewDirTangent(tbn, fs_in.v_view_vec);
            texCoord = parallaxOcclusionMapping(texCoord, view_dir_tangent);
        }

        if (material.normal_exist) {
            normal = getFragNormalInWorld(tbn, texCoord);
        }
    }

    if (material.pure_color) {
        out_color = vec4(material.color, 1);
    } else {
        vec4 ambient = vec4(0);
        vec4 diffuse = vec4(0);
        vec4 specular = vec4(0);
        vec4 env_reflect = vec4(0);
        vec4 env_refract = vec4(0);

        if (material.diffuse_exist) {
            ambient = texture(material.diffuse_texture, texCoord);
            diffuse = ambient;
        }

        if (material.specular_exist) {
            specular = texture(material.specular_texture, texCoord);
        }

        if (diffuse.a < 0.01 && specular.a < 0.01)
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
        vec3 point_color = calcPointLights(normal, to_camera, view_distance, ambient.rgb, diffuse.rgb, specular.rgb);
        vec3 spot_color = calcSpotLights(normal, to_camera, ambient.rgb, diffuse.rgb, specular.rgb);

        out_color = vec4(directional_color + point_color + spot_color + env_reflect.rgb + env_refract.rgb, diffuse.a);
    }
}
