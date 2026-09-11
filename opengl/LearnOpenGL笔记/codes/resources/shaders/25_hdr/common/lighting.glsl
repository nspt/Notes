layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_bright;

uniform float bloom_threshold;

mat3 getTBNMatrix(vec3 tangent, vec3 normal)
{
    vec3 T = normalize(tangent);
    vec3 N = normalize(normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    return mat3(T, B, N);
}

vec3 getToCameraTangent(mat3 tbn, vec3 to_camera)
{
    return normalize(vec3(
        dot(tbn[0], to_camera),
        dot(tbn[1], to_camera),
        dot(tbn[2], to_camera)
    ));
}

vec2 parallaxOcclusionMapping(vec2 uv, vec3 to_camera)
{
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), to_camera)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = to_camera.xy * material.height_scale / to_camera.z;
    vec2 deltaUv = P / numLayers;

    vec2 currentUv = uv;
    float currentDepthMapValue = texture(material.height_map, currentUv).r;

    for (float i = 0.0; i < numLayers; i += 1.0) {
        if (currentLayerDepth >= currentDepthMapValue)
            break;
        currentUv -= deltaUv;
        currentDepthMapValue = texture(material.height_map, currentUv).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevUv = currentUv + deltaUv;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(material.height_map, prevUv).r - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth + 1e-6);
    return mix(currentUv, prevUv, weight);
}

vec3 getFragNormalInWorld(mat3 tbn, vec2 uv)
{
    vec3 tangent_normal = texture(material.normal_map, uv).xyz * 2.0 - 1.0;
    return normalize(tbn * tangent_normal);
}

float dirShadowLitFactor(vec4 light_space_frag_pos, sampler2D depth_map,
                         vec2 texel_size, float bias)
{
    vec3 coords = light_space_frag_pos.xyz / light_space_frag_pos.w;
    coords = coords * 0.5 + 0.5;
    if (coords.z > 1.0)
        return 0.0;

    float cur_depth = coords.z;
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest_depth = texture(
                depth_map,
                coords.xy + vec2(x, y) * texel_size
            ).r;
            lit += cur_depth - bias > closest_depth ? 0.0 : 1.0;
        }
    }
    return lit / 9.0;
}

float omniShadowLitFactor(vec3 to_light_vec, samplerCube depth_map,
                          float bias, float far_plane, float to_camera_distance)
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
    float disk_radius = (1.0 + (to_camera_distance / far_plane)) / 25.0;

    float lit = 0.0;
    for (int i = 0; i < 20; ++i) {
        float closest_depth = texture(depth_map, light_to_frag + sample_offsets[i] * disk_radius).r;
        closest_depth *= far_plane;
        lit += cur_depth - bias > closest_depth ? 0.0 : 1.0;
    }
    return lit / 20.0;
}

vec3 calcDirectionalLights(vec3 normal, vec3 to_camera, vec3 ambient, vec3 diffuse, vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.x; ++i) {
        vec3 light_dir = lightData.directional[i].direction.xyz;
        float bias = dot(normal, light_dir) * shadowMap.directional[i].bias.slope_bias
                   + shadowMap.directional[i].bias.min_bias;
        float lit = (i < shadowMap.counts.x)
            ? dirShadowLitFactor(fs_in.v_dls_pos[i],
                                 shadowMap.directional[i].map,
                                 shadowMap.directional[i].texel_size, bias)
            : 1.0;
        float diff = max(dot(normal, -light_dir), 0.0);
        vec3 halfway = normalize(-light_dir + to_camera);
        float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
        result += lightData.directional[i].ambient.xyz * ambient;
        result += lightData.directional[i].diffuse.xyz * diff * diffuse * lit;
        result += lightData.directional[i].specular.xyz * spec * specular * lit;
    }
    return result;
}

vec3 calcPointLights(vec3 normal, vec3 to_camera, float to_camera_distance,
                     vec3 ambient, vec3 diffuse, vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.y; ++i) {
        vec3 to_light_vec = fs_in.v_ws_to_point_light[i];
        float distance = length(to_light_vec);
        vec3 to_light = to_light_vec / distance;
        vec3 light_dir = -to_light;
        float bias = dot(normal, light_dir) * shadowMap.point[i].bias.slope_bias
                   + shadowMap.point[i].bias.min_bias;
        float diff = max(dot(normal, to_light), 0.0);
        vec3 halfway = normalize(to_light + to_camera);
        float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
        float attenuation = 1.0 / (lightData.point[i].attenuation.x
            + lightData.point[i].attenuation.y * distance
            + lightData.point[i].attenuation.z * (distance * distance));
        float lit = (i < shadowMap.counts.y)
            ? omniShadowLitFactor(to_light_vec, shadowMap.point[i].map, bias,
                                  shadowMap.point[i].far_plane, to_camera_distance)
            : 1.0;
        vec3 contrib = lightData.point[i].ambient.xyz * ambient
                     + lightData.point[i].diffuse.xyz * diff * diffuse * lit
                     + lightData.point[i].specular.xyz * spec * specular * lit;
        result += contrib * attenuation;
    }
    return result;
}

vec3 calcSpotLights(vec3 normal, vec3 to_camera, vec3 ambient, vec3 diffuse, vec3 specular)
{
    vec3 result = vec3(0.0);
    for (int i = 0; i < lightData.counts.z; ++i) {
        vec3 to_light_vec = fs_in.v_ws_to_spot_light[i];
        float distance = length(to_light_vec);
        vec3 to_light = to_light_vec / distance;
        vec3 light_dir = lightData.spot[i].direction.xyz;
        float bias = dot(normal, light_dir) * shadowMap.spot[i].bias.slope_bias
                   + shadowMap.spot[i].bias.min_bias;
        float diff = max(dot(normal, to_light), 0.0);
        vec3 halfway = normalize(to_light + to_camera);
        float spec = pow(max(dot(normal, halfway), 0.0), material.shininess);
        float attenuation = 1.0 / (lightData.spot[i].attenuation.x
            + lightData.spot[i].attenuation.y * distance
            + lightData.spot[i].attenuation.z * (distance * distance));
        float outer_cutoff = lightData.spot[i].cutoff.y;
        float inv_epsilon = lightData.spot[i].cutoff.z;
        float theta = dot(to_light, -light_dir);
        float intensity = inv_epsilon > 0.0
            ? clamp((theta - outer_cutoff) * inv_epsilon, 0.0, 1.0)
            : (theta > outer_cutoff ? 1.0 : 0.0);
        float lit = (i < shadowMap.counts.z)
            ? dirShadowLitFactor(fs_in.v_sls_pos[i],
                                 shadowMap.spot[i].map,
                                 shadowMap.spot[i].texel_size, bias)
            : 1.0;
        vec3 contrib = lightData.spot[i].ambient.xyz * ambient
                     + lightData.spot[i].diffuse.xyz * diff * diffuse * lit
                     + lightData.spot[i].specular.xyz * spec * specular * lit;
        result += contrib * attenuation * intensity;
    }
    return result;
}

vec4 shadeFragment()
{
    vec3 normal = normalize(fs_in.v_ws_normal);
    vec2 uv = fs_in.v_uv_coord;
    vec3 to_camera = normalize(fs_in.v_ws_to_camera);
    float to_camera_distance = length(fs_in.v_ws_to_camera);

    if (material.height_exist || material.normal_exist) {
        mat3 tbn = getTBNMatrix(fs_in.v_ws_tangent, fs_in.v_ws_normal);
        if (material.height_exist) {
            uv = parallaxOcclusionMapping(uv, getToCameraTangent(tbn, fs_in.v_ws_to_camera));
            if (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0)
                discard;
        }
        if (material.normal_exist) {
            normal = getFragNormalInWorld(tbn, uv);
        }
    }

    if (material.pure_color) {
        return vec4(material.color, 1.0);
    }

    vec4 ambient = vec4(0.0);
    vec4 diffuse = vec4(0.0);
    vec4 specular = vec4(0.0);
    vec4 env_reflect = vec4(0.0);
    vec4 env_refract = vec4(0.0);

    if (material.diffuse_exist) {
        ambient = texture(material.diffuse_texture, uv);
        diffuse = ambient;
    }
    if (material.specular_exist) {
        specular = texture(material.specular_texture, uv);
    }
    if (diffuse.a < 0.01 && specular.a < 0.01)
        discard;

    if (material.reflect_cube_exist) {
        env_reflect = texture(material.reflect_cube_texture, reflect(-to_camera, normal));
    }
    if (material.refract_cube_exist) {
        env_refract = texture(material.refract_cube_texture,
                              refract(-to_camera, normal, material.refract_ratio));
    }

    vec3 lit = calcDirectionalLights(normal, to_camera, ambient.rgb, diffuse.rgb, specular.rgb)
             + calcPointLights(normal, to_camera, to_camera_distance, ambient.rgb, diffuse.rgb, specular.rgb)
             + calcSpotLights(normal, to_camera, ambient.rgb, diffuse.rgb, specular.rgb);
    return vec4(lit + env_reflect.rgb + env_refract.rgb, diffuse.a);
}

void emitShadedFragment()
{
    vec4 color = shadeFragment();
    out_color = color;
    float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    out_bright = brightness > bloom_threshold ? vec4(color.rgb, 1.0) : vec4(0.0);
}
