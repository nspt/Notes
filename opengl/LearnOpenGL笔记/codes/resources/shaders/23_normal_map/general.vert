#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 3) in vec3 a_tangent;
layout (location = 4) in mat4 a_model;

out VS_OUT {
    vec3 v_world_pos;
    vec3 v_world_normal;
    vec2 v_tex_coord;
    vec3 v_world_tangent;
    vec3 v_view_vec;
    vec3 v_to_point_light[8];
    vec3 v_to_spot_light[4];
    vec4 v_directional_light_space_pos[2];
    vec4 v_spot_light_space_pos[4];
} vs_out;


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

void main()
{
    mat3 normal_matrix = mat3(transpose(inverse(a_model)));
    vec4 world_pos = a_model * vec4(a_pos, 1.0);
    gl_Position = cam_data.projection * cam_data.view * world_pos;
    vs_out.v_world_pos = world_pos.xyz;
    vs_out.v_world_normal = normal_matrix * a_normal;
    vs_out.v_tex_coord = a_tex_coord;
    vs_out.v_world_tangent = normal_matrix * a_tangent;

    for (int i = 0; i < 2; ++i) {
        vs_out.v_directional_light_space_pos[i] =
            lightData.directional[i].light_space_transform * world_pos;
    }
    for (int i = 0; i < 4; ++i) {
        vs_out.v_spot_light_space_pos[i] =
            lightData.spot[i].light_space_transform * world_pos;
    }

    vs_out.v_view_vec = cam_data.pos.xyz - world_pos.xyz;
    for (int i = 0; i < lightData.counts.y; ++i) {
        vs_out.v_to_point_light[i] =
            lightData.point_light[i].position.xyz - world_pos.xyz;
    }
    for (int i = 0; i < lightData.counts.z; ++i) {
        vs_out.v_to_spot_light[i] =
            lightData.spot[i].position.xyz - world_pos.xyz;
    }
}
