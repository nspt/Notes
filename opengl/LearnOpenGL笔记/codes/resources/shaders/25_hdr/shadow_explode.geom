#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec2 v_uv_coord;
} gs_in[];

out vec2 v_uv_coord;

uniform mat4 light_space_transform;
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
        vec4 world_pos = explode(gl_in[i].gl_Position, normal);
        gl_Position = light_space_transform * world_pos;
        v_uv_coord = gs_in[i].v_uv_coord;
        EmitVertex();
    }
    EndPrimitive();
}
