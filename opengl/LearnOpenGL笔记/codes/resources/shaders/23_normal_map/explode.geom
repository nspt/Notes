#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
    vec3 v_world_tangent;
} gs_in[];

out GS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
    vec3 v_world_tangent;
} gs_out;

layout (std140) uniform CamData {
    mat4 view;
    mat4 projection;
    vec4 pos;
} cam_data;

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
    // 视图空间法线转到世界空间，同步更新世界坐标供光照 / 阴影采样
    vec3 world_normal = normalize(mat3(transpose(cam_data.view)) * view_normal);

    for (int i = 0; i < 3; ++i) {
        vec4 view_pos = explode(gl_in[i].gl_Position, view_normal);
        gl_Position = cam_data.projection * view_pos;
        gs_out.v_world_pos = gs_in[i].v_world_pos + world_normal * explode_magnitude;
        gs_out.v_world_normal = gs_in[i].v_world_normal;
        gs_out.v_tex_coord = gs_in[i].v_tex_coord;
        gs_out.v_world_tangent = gs_in[i].v_world_tangent;
        EmitVertex();
    }
    EndPrimitive();
}
