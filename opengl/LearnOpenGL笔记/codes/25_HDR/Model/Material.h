#pragma once

#include "../Textures/Texture.h"
#include "glm/glm.hpp"
#include <optional>

class Material {
public:
    std::optional<Texture> diffuse_;
    std::optional<Texture> specular_;
    std::optional<Texture> normal_;
    std::optional<Texture> height_;
    std::optional<Texture> skybox_;
    std::optional<Texture> quad_texture_; // 后处理 quad 的唯一输入
    float height_scale_{ 0.1f };
    float shininess_{ 32.0f };
    bool pure_color_{ false };
    glm::vec3 color_{ 1.0f };

    bool operator==(const Material &rhs) const noexcept = default;
};
