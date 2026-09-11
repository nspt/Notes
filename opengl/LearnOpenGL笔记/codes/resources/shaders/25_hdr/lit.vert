#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 3) in vec3 a_tangent;
layout (location = 4) in mat4 a_model;

out VS_OUT {
    vec3 v_ws_pos;
    vec3 v_ws_normal;
    vec2 v_uv_coord;
    vec3 v_ws_tangent;
    vec3 v_ws_to_camera;
    vec3 v_ws_to_point_light[8];
    vec3 v_ws_to_spot_light[4];
    vec4 v_dls_pos[2]; // directional light space
    vec4 v_sls_pos[4]; // spot light space
} vs_out;

#include "common/cam_data.glsl"
#include "common/light_data.glsl"
#include "common/shadow_map.glsl"

void main()
{
    mat3 normal_matrix = mat3(transpose(inverse(a_model)));
    vec4 world_pos = a_model * vec4(a_pos, 1.0);
    gl_Position = cam_data.projection * cam_data.view * world_pos;

    vs_out.v_ws_pos = world_pos.xyz;
    vs_out.v_ws_normal = normal_matrix * a_normal;
    vs_out.v_uv_coord = a_tex_coord;
    vs_out.v_ws_tangent = normal_matrix * a_tangent;
    vs_out.v_ws_to_camera = cam_data.pos.xyz - world_pos.xyz;

    for (int i = 0; i < 2; ++i) {
        vs_out.v_dls_pos[i] = shadowMap.directional[i].transform * world_pos;
    }
    for (int i = 0; i < 4; ++i) {
        vs_out.v_sls_pos[i] = shadowMap.spot[i].transform * world_pos;
    }
    for (int i = 0; i < lightData.counts.y; ++i) {
        vs_out.v_ws_to_point_light[i] = lightData.point[i].position.xyz - world_pos.xyz;
    }
    for (int i = 0; i < lightData.counts.z; ++i) {
        vs_out.v_ws_to_spot_light[i] = lightData.spot[i].position.xyz - world_pos.xyz;
    }
}
