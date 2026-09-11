#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 v_ws_pos;
    vec3 v_ws_normal;
    vec2 v_uv_coord;
    vec3 v_ws_tangent;
    vec3 v_ws_to_camera;
    vec3 v_ws_to_point_light[8];
    vec3 v_ws_to_spot_light[4];
    vec4 v_dls_pos[2]; // directional light space
    vec4 v_sls_pos[4]; // spot light space
} gs_in[];

out GS_OUT {
    vec3 v_ws_pos;
    vec3 v_ws_normal;
    vec2 v_uv_coord;
    vec3 v_ws_tangent;
    vec3 v_ws_to_camera;
    vec3 v_ws_to_point_light[8];
    vec3 v_ws_to_spot_light[4];
    vec4 v_dls_pos[2]; // directional light space
    vec4 v_sls_pos[4]; // spot light space
} gs_out;

#include "common/cam_data.glsl"
#include "common/light_data.glsl"
#include "common/shadow_map.glsl"

uniform float explode_magnitude;

vec3 getNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(b, a));
}

vec4 explode(vec4 position, vec3 normal)
{
    return position + vec4(normal * explode_magnitude, 0.0);
}

void main()
{
    vec3 view_normal = getNormal();
    vec3 world_normal = normalize(mat3(transpose(cam_data.view)) * view_normal);

    for (int i = 0; i < 3; ++i) {
        vec4 view_pos = explode(gl_in[i].gl_Position, view_normal);
        gl_Position = cam_data.projection * view_pos;
        gs_out.v_ws_pos = gs_in[i].v_ws_pos + world_normal * explode_magnitude;
        gs_out.v_ws_normal = gs_in[i].v_ws_normal;
        gs_out.v_uv_coord = gs_in[i].v_uv_coord;
        gs_out.v_ws_tangent = gs_in[i].v_ws_tangent;

        vec4 exploded_world_pos = vec4(gs_out.v_ws_pos, 1.0);
        for (int j = 0; j < 2; ++j) {
            gs_out.v_dls_pos[j] = shadowMap.directional[j].transform * exploded_world_pos;
        }
        for (int j = 0; j < 4; ++j) {
            gs_out.v_sls_pos[j] = shadowMap.spot[j].transform * exploded_world_pos;
        }

        gs_out.v_ws_to_camera = cam_data.pos.xyz - gs_out.v_ws_pos;
        for (int j = 0; j < lightData.counts.y; ++j) {
            gs_out.v_ws_to_point_light[j] = lightData.point[j].position.xyz - gs_out.v_ws_pos;
        }
        for (int j = 0; j < lightData.counts.z; ++j) {
            gs_out.v_ws_to_spot_light[j] = lightData.spot[j].position.xyz - gs_out.v_ws_pos;
        }
        EmitVertex();
    }
    EndPrimitive();
}
