#pragma once

#include <array>
#include <format>
#include <stdexcept>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "FrameBuffer.h"
#include "ShaderProgram.h"
#include "Texture2D.h"

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
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point[8];
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

inline void initLightData(LightData &data)
{
    // 1. directional
    data.counts.x = 1;
    data.directional[0].direction_ = glm::normalize(glm::vec4{ -1, -1, -1, 0 });
    data.directional[0].ambient_ = glm::vec4{ 0.05 };
    data.directional[0].diffuse_ = glm::vec4{ 0.3 };
    data.directional[0].specular_ = glm::vec4{ 0.5 };
    data.directional[0].light_space_transform_ = calcDirectionalLightSpaceTransform(
        glm::vec3(data.directional[0].direction_), 5.5f, 1.0f, 500.0f
    );

    // 2. point
    data.counts.y = 0;
    data.point[0].pos_ = glm::vec4{ 5, 0, 0, 1 };
    data.point[0].ambient_ = glm::vec4{ 0.2 };
    data.point[0].diffuse_ = glm::vec4{ 0.5 };
    data.point[0].specular_ = glm::vec4{ 1.0 };
    data.point[0].attenuation = glm::vec4{ 1.0, 0.027, 0.0028, 0.0 };
    {
        const glm::vec3 pos = glm::vec3(data.point[0].pos_);
        const glm::vec3 dir = glm::normalize(glm::vec3{ 0.0f } - pos);
        data.point[0].light_space_transform_ = calcSpotLightSpaceTransform(
            pos, dir, glm::cos(glm::radians(45.0f)), 0.1f, 50.0f
        );
    }

    // 3. spot
    data.counts.z = 1;
    data.spot[0].pos_ = glm::vec4{ 0, 10, 10, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[0].pos_));
        // inner/outer：余弦值，略放宽锥角，避免只有中心一点亮
        data.spot[0].direction_inner_ = glm::vec4{ dir, glm::cos(glm::radians(22.5f)) };
    }
    data.spot[0].ambient_ = glm::vec4{ 0.1 };
    data.spot[0].diffuse_ = glm::vec4{ 1.0 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    // 距离约 14（灯到原点），过强的 quadratic 会把光压得很暗
    data.spot[0].attenuation_outter_ = glm::vec4{ 1.0, 0.0009, 0.00032, glm::cos(glm::radians(37.5f)) };
    data.spot[0].light_space_transform_ = calcSpotLightSpaceTransform(
        glm::vec3(data.spot[0].pos_),
        glm::vec3(data.spot[0].direction_inner_),
        data.spot[0].attenuation_outter_.w,
        0.1f, 100.0f
    );

    data.spot[1].pos_ = glm::vec4{ 10, 10, 0, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[1].pos_));
        data.spot[1].direction_inner_ = glm::vec4{ dir, glm::cos(glm::radians(12.5f)) };
    }
    data.spot[1].ambient_ = glm::vec4{ 0.1 };
    data.spot[1].diffuse_ = glm::vec4{ 1.0 };
    data.spot[1].specular_ = glm::vec4{ 1.0 };
    data.spot[1].attenuation_outter_ = glm::vec4{ 1.0, 0.09, 0.032, glm::cos(glm::radians(17.5f)) };
    data.spot[1].light_space_transform_ = calcSpotLightSpaceTransform(
        glm::vec3(data.spot[1].pos_),
        glm::vec3(data.spot[1].direction_inner_),
        data.spot[1].attenuation_outter_.w,
        0.1f, 50.0f
    );
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

inline FrameBuffer createShadowMapFBO(const Texture2D &depth_texture)
{
    FrameBuffer fbo;
    fbo.attachTexture(GL_DEPTH_ATTACHMENT, depth_texture);
    if (!fbo.isCompleted()) {
        throw std::runtime_error{ "Shadow FBO is not completed" };
    }
    return fbo;
}

inline void bindShadowMapSamplers(
    ShaderProgram &shader,
    const glm::ivec4 &shadow_counts,
    const std::array<Texture2D, 2> &directional_maps,
    const std::array<Texture2D, 4> &spot_maps,
    const std::array<float, 2> &directional_bias,
    const std::array<float, 4> &spot_bias,
    const std::array<float, 8> &point_bias,
    unsigned first_unit)
{
    shader.setIVec4("shadowMap.counts", shadow_counts);

    // sampler 只要在 shader 里仍是 active，就必须绑到完整纹理；
    // counts 跳过采样后仍可能被驱动视为 active，未使用的槽用已有 shadow map 占位即可
    const Texture2D &fallback = directional_maps[0];

    unsigned unit = first_unit;
    for (int i = 0; i < 2; ++i) {
        const Texture2D &map = i < shadow_counts.x ? directional_maps[i] : fallback;
        map.bind(unit);
        shader.setInt(std::format("shadowMap.directional[{}]", i), static_cast<int>(unit));
        shader.setFloat(std::format("shadowMap.directional_bias[{}]", i), directional_bias[i]);
        ++unit;
    }
    for (int i = 0; i < 4; ++i) {
        const Texture2D &map = i < shadow_counts.z ? spot_maps[i] : fallback;
        map.bind(unit);
        shader.setInt(std::format("shadowMap.spot[{}]", i), static_cast<int>(unit));
        shader.setFloat(std::format("shadowMap.spot_bias[{}]", i), spot_bias[i]);
        ++unit;
    }
    for (int i = 0; i < 8; ++i) {
        fallback.bind(unit);
        shader.setInt(std::format("shadowMap.point_light[{}]", i), static_cast<int>(unit));
        shader.setFloat(std::format("shadowMap.point_bias[{}]", i), point_bias[i]);
        ++unit;
    }
}
