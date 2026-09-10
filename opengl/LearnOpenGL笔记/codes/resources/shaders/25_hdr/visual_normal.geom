#version 330 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
} gs_in[];

out GS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
} gs_out;

#include "common/cam_data.glsl"

void main()
{
    for (int i = 0; i < 3; ++i) {
        gs_out.v_world_pos = gs_in[i].v_world_pos;
        gs_out.v_world_normal = gs_in[i].v_world_normal;
        gs_out.v_tex_coord = gs_in[i].v_tex_coord;
        gl_Position = cam_data.projection * gl_in[i].gl_Position;
        EmitVertex();
        vec4 normal_end = vec4(gs_in[i].v_world_pos + (0.1 * normalize(gs_in[i].v_world_normal)), 1.0);
        gl_Position = cam_data.projection * cam_data.view * normal_end;
        EmitVertex();
        EndPrimitive();
    }
}
