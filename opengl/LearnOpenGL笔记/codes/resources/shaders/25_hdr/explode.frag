#version 330 core

in GS_OUT {
    vec3 v_ws_pos;
    vec3 v_ws_normal;
    vec2 v_uv_coord;
    vec3 v_ws_tangent;
    vec3 v_ws_to_camera;
    vec3 v_ws_to_point_light[8];
    vec3 v_ws_to_spot_light[4];
    vec4 v_dls_pos[2]; // directional light space
    vec4 v_sls_pos[4]; // spot light space
} fs_in;

#include "common/cam_data.glsl"
#include "common/light_data.glsl"
#include "common/shadow_map.glsl"
#include "common/material.glsl"
#include "common/lighting.glsl"

void main()
{
    emitShadedFragment();
}
