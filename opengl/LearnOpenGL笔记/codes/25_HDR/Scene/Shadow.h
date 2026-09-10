#pragma once

#include <array>
#include <stdexcept>
#include "Light.h"
#include "../Textures/Texture2D.h"
#include "../Textures/TextureCubeMap.h"
#include "../FrameBuffer/FrameBuffer.h"

struct ShadowBias {
    float min_bias = 0.0f;
    float slope_bias = 0.0f;
};

struct DirLightShadowData {
    glm::mat4 transform;
    ShadowBias bias;
};

struct DirLightShadowRes {
    DirLightShadowData params;
    Texture2D texture;
    FrameBuffer fb;
};

struct OmniLightShadowData {
    glm::mat4 transforms[6];
    ShadowBias bias;
    float far_plane;
    glm::vec4 position;
};

struct OmniLightShadowRes {
    OmniLightShadowData params;
    TextureCubeMap texture;
    FrameBuffer fb;
};

struct ShadowData {
    std::vector<DirLightShadowData> directional_;
    std::vector<DirLightShadowData> spot_;
    std::vector<OmniLightShadowData> point_;
};

struct ShadowResources {
    std::vector<DirLightShadowRes> directional_;
    std::vector<DirLightShadowRes> spot_;
    std::vector<OmniLightShadowRes> point_;
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
    Texture2D tex{ size, size, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, nullptr, GL_FLOAT };
    tex.bind(0);
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
    TextureCubeMap tex{ size, size, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT };
    tex.bind(0);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tex;
}

inline FrameBuffer createShadowMapFBO(const Texture2D &depth_texture)
{
    FrameBuffer fbo;
    fbo.attachTexture(GL_DEPTH_ATTACHMENT, depth_texture);
    fbo.drawBuffer(GL_NONE);
    fbo.readBuffer(GL_NONE);
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
    fbo.drawBuffer(GL_NONE);
    fbo.readBuffer(GL_NONE);
    if (!fbo.isCompleted()) {
        throw std::runtime_error{ "Shadow cubemap FBO is not completed" };
    }
    fbo.unbind();
    return fbo;
}