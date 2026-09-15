#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <stdexcept>
#include "../Textures/Texture2D.h"
#include "../Textures/TextureCubeMap.h"
#include "../FrameBuffer/FrameBuffer.h"

#define MAX_DIRECTIONAL_LIGHT 2
#define MAX_POINT_LIGHT 8
#define MAX_SPOT_LIGHT 4

struct alignas(16) DirectionalLight {
    glm::vec4 direction_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 shadow_; // x: min bias, y: slope bias, z: 1.0 / tex_width, w: 1.0 / tex_height
    glm::mat4 transform_;
};

struct alignas(16) SpotLight {
    glm::vec4 position_;
    glm::vec4 direction_;     // xyz
    glm::vec4 cutoff_;        // x: inner cos, y: outer cos, z: inv_epsilon
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 attenuation_;   // xyz: const/linear/quad
    glm::vec4 shadow_; // x: min bias, y: slope bias, z: 1.0 / tex_width, w: 1.0 / tex_height
    glm::mat4 transform_;
};

struct alignas(16) PointLight {
    glm::vec4 position_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 attenuation_;   // xyz: const/linear/quad, w: far_plane
    glm::vec4 shadow_; // x: min bias, y: slope bias, z: tex width, w: tex height
};

struct alignas(16) LightData {
    glm::ivec4 counts_{ 0 }; // x: directional, y: point, z: spot
    DirectionalLight directional_[MAX_DIRECTIONAL_LIGHT];
    SpotLight spot_[MAX_SPOT_LIGHT];
    PointLight point_[MAX_POINT_LIGHT];
};

struct ShadowMaps {
    struct DirShadowResource {
        Texture2D texture_;
        FrameBuffer fb_;
    };
    struct OmniShadowResource {
        TextureCubeMap texture_;
        FrameBuffer fb_;
    };
    DirShadowResource directional_[MAX_DIRECTIONAL_LIGHT];
    DirShadowResource spot_[MAX_SPOT_LIGHT];
    OmniShadowResource point_[MAX_POINT_LIGHT];
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

inline Texture2D createShadowMapTexture(int width, int height)
{
    Texture2D tex{ width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, nullptr, GL_FLOAT };
    tex.setFilterMode(GL_NEAREST, GL_NEAREST);
    tex.setWrapMode(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
    tex.bind(0);
    const float border_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    return tex;
}

inline TextureCubeMap createShadowMapTextureCube(int width, int height)
{
    TextureCubeMap tex{ width, height, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_FLOAT };
    tex.setFilterMode(GL_NEAREST, GL_NEAREST);
    tex.setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
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