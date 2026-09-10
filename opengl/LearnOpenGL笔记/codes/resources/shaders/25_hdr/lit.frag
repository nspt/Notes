#version 330 core

in VS_OUT {
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

#include "common/cam_data.glsl"
#include "common/light_data.glsl"
#include "common/shadow_map.glsl"
#include "common/material.glsl"
#include "common/lighting.glsl"

void main()
{
    emitShadedFragment();
}
