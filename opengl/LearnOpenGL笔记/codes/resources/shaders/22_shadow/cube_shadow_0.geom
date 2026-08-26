#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 light_space_transform[6];

in VS_OUT {
    vec2 v_tex_coord;
} gs_in[];

out vec2 v_tex_coord;
out vec3 v_frag_pos;

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle vertex
        {
            v_frag_pos = gl_in[i].gl_Position.xyz;
            gl_Position = light_space_transform[face] * gl_in[i].gl_Position;
            v_tex_coord = gs_in[i].v_tex_coord;
            EmitVertex();
        }    
        EndPrimitive();
    }
}  