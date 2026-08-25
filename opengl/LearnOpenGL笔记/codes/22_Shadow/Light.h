#pragma once

#include <array>
#include <format>
#include <stdexcept>
#include <utility>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "FrameBuffer.h"
#include "ShaderProgram.h"
#include "Texture.h"
#include "Texture2D.h"
#include "TextureCubeMap.h"

#define MAX_DIRECTIONAL_LIGHT 2
#define MAX_POINT_LIGHT 8
#define MAX_SPOT_LIGHT 4

struct alignas(16) DirectionalLight {
    glm::vec4 direction_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::mat4 light_space_transform_;
};

struct alignas(16) PointLight {
    glm::vec4 pos_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 attenuation;   // x: constant, y: linear, z: quadratic, w: reserve
    glm::mat4 light_space_transform_;
};

struct alignas(16) SpotLight {
    glm::vec4 pos_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 direction_inner_; // xyz:direction, w:inner cutoff
    glm::vec4 attenuation_outter_;   // x: constant, y: linear, z: quadratic, w: outer cutoff
    glm::mat4 light_space_transform_;
};

struct LightData {
    glm::ivec4 counts{ 0 }; // x: directional, y: point, z: spot
    DirectionalLight directional[MAX_DIRECTIONAL_LIGHT];
    SpotLight spot[MAX_SPOT_LIGHT];
    PointLight point[MAX_POINT_LIGHT];
};

struct ShadowData {
    glm::ivec4 counts{ 0 }; // x: directional, y: point, z: spot
    std::pair<Texture2D, float> directional[MAX_DIRECTIONAL_LIGHT];
    std::pair<Texture2D, float> spot[MAX_SPOT_LIGHT];
    std::pair<TextureCubeMap, float> point[MAX_POINT_LIGHT];
    float point_far[MAX_POINT_LIGHT]{};

    void apply(ShaderProgram &shader, unsigned first_unit = 0) const
    {
        shader.setIVec4("shadowMap.counts", counts);

        unsigned unit = first_unit;
        for (int i = 0; i < counts.x; ++i) {
            directional[i].first.bind(unit);
            shader.setInt(std::format("shadowMap.directional[{}]", i), static_cast<int>(unit));
            shader.setFloat(std::format("shadowMap.directional_bias[{}]", i), directional[i].second);
            ++unit;
        }
        for (int i = 0; i < counts.z; ++i) {
            spot[i].first.bind(unit);
            shader.setInt(std::format("shadowMap.spot[{}]", i), static_cast<int>(unit));
            shader.setFloat(std::format("shadowMap.spot_bias[{}]", i), spot[i].second);
            ++unit;
        }
        for (int i = 0; i < counts.y; ++i) {
            point[i].first.bind(unit);
            shader.setInt(std::format("shadowMap.point_light[{}]", i), static_cast<int>(unit));
            shader.setFloat(std::format("shadowMap.point_bias[{}]", i), point[i].second);
            shader.setFloat(std::format("shadowMap.point_far[{}]", i), point_far[i]);
            ++unit;
        }
    }
};


inline glm::mat4 calcDirectionalLightSpaceTransform(
    glm::vec3 light_direction, float ortho_half_extent, float near_plane, float far_plane)
{
    const glm::vec3 dir = glm::normalize(light_direction);
    const glm::vec3 light_pos = -dir * (far_plane * 0.5f);
    const glm::mat4 light_view = glm::lookAt(light_pos, glm::vec3{ 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });
    const glm::mat4 light_projection = glm::ortho(
        -ortho_half_extent, ortho_half_extent,
        -ortho_half_extent, ortho_half_extent,
        near_plane, far_plane
    );
    return light_projection * light_view;
}

inline glm::mat4 calcSpotLightSpaceTransform(
    glm::vec3 pos, glm::vec3 direction, float outer_cutoff, float near_plane, float far_plane)
{
    const glm::mat4 light_projection = glm::perspective(glm::acos(outer_cutoff) * 2.0f, 1.0f, near_plane, far_plane);
    const glm::mat4 light_view = glm::lookAt(pos, pos + direction, glm::vec3{ 0.0f, 1.0f, 0.0f });
    return light_projection * light_view;
}

// 点光源阴影：6 个 90° 透视，顺序对应 GL_TEXTURE_CUBE_MAP_POSITIVE_X ... NEGATIVE_Z
inline std::array<glm::mat4, 6> calcPointLightSpaceTransforms(
    glm::vec3 light_pos, float near_plane, float far_plane)
{
    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, near_plane, far_plane);
    return {
        projection * glm::lookAt(light_pos, light_pos + glm::vec3{  1.0f,  0.0f,  0.0f }, glm::vec3{ 0.0f, -1.0f,  0.0f }),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3{ -1.0f,  0.0f,  0.0f }, glm::vec3{ 0.0f, -1.0f,  0.0f }),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3{  0.0f,  1.0f,  0.0f }, glm::vec3{ 0.0f,  0.0f,  1.0f }),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3{  0.0f, -1.0f,  0.0f }, glm::vec3{ 0.0f,  0.0f, -1.0f }),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3{  0.0f,  0.0f,  1.0f }, glm::vec3{ 0.0f, -1.0f,  0.0f }),
        projection * glm::lookAt(light_pos, light_pos + glm::vec3{  0.0f,  0.0f, -1.0f }, glm::vec3{ 0.0f, -1.0f,  0.0f }),
    };
}

inline Texture2D createShadowMapTexture(int size)
{
    Texture2D tex{ size, size, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, nullptr };
    tex.bind(0);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    return tex;
}

inline TextureCubeMap createShadowMapTextureCube(int size)
{
    // 构造时 faces 为空不会分配 6 面存储，这里补齐为 depth cubemap
    TextureCubeMap tex{ size, size, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT };
    tex.bind(0);
    for (unsigned i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0,
            GL_DEPTH_COMPONENT,
            size, size,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0); // 避免 unit0 残留 depth cubemap 被 skybox 误采样
    return tex;
}

inline FrameBuffer createShadowMapFBO(const Texture2D &depth_texture)
{
    FrameBuffer fbo;
    fbo.attachTexture(GL_DEPTH_ATTACHMENT, depth_texture);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (!fbo.isCompleted()) {
        throw std::runtime_error{ "Shadow FBO is not completed" };
    }
    fbo.unbind();
    return fbo;
}

inline FrameBuffer createShadowMapFBO(const TextureCubeMap &depth_cubemap)
{
    FrameBuffer fbo;
    fbo.attachTexture(GL_DEPTH_ATTACHMENT, depth_cubemap);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (!fbo.isCompleted()) {
        throw std::runtime_error{ "Shadow cubemap FBO is not completed" };
    }
    fbo.unbind();
    return fbo;
}
