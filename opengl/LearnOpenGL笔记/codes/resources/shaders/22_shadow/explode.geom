#version 330 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
    vec4 v_directional_light_space_pos[2];
    vec4 v_spot_light_space_pos[4];
} gs_in[];

out GS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
    vec4 v_directional_light_space_pos[2];
    vec4 v_spot_light_space_pos[4];
} gs_out;

layout (std140) uniform CamData {
    mat4 view;
    mat4 projection;
    vec4 pos;
} cam_data;

struct DirectionalLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    mat4 light_space_transform;
};

struct SpotLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 direction;
    vec4 attenuation;
    mat4 light_space_transform;
};

struct PointLight {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 attenuation;
};

layout (std140) uniform LightData {
    ivec4 counts;
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point_light[8];
} lightData;

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

        vec4 exploded_world_pos = vec4(gs_out.v_world_pos, 1.0);
        for (int j = 0; j < 2; ++j) {
            gs_out.v_directional_light_space_pos[j] =
                lightData.directional[j].light_space_transform * exploded_world_pos;
        }
        for (int j = 0; j < 4; ++j) {
            gs_out.v_spot_light_space_pos[j] =
                lightData.spot[j].light_space_transform * exploded_world_pos;
        }
        EmitVertex();
    }
    EndPrimitive();
}
