#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

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

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

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
    vec3 normal = getNormal();
    
    for (int i = 0; i < 3; ++i) {
        gl_Position = projection * explode(gl_in[i].gl_Position, normal);
        gs_out.v_world_pos = gs_in[i].v_world_pos;
        gs_out.v_world_normal = gs_in[i].v_world_normal;
        gs_out.v_tex_coord = gs_in[i].v_tex_coord;
        EmitVertex();
    }
    EndPrimitive();
}