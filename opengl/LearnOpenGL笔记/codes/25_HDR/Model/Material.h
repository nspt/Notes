#pragma once

#include "../Textures/Texture.h"
#include "../Renderer/ShaderProgram.h"
#include "glm/glm.hpp"
#include <optional>

class Material {
public:
    std::optional<Texture> diffuse_;
    std::optional<Texture> specular_;
    std::optional<Texture> normal_;
    std::optional<Texture> height_;
    float height_scale_{ 0.1f };
    float shininess_{ 32.0f };
    bool pure_color_{ false };
    glm::vec3 color_{ 1.0f };

    bool operator==(const Material &rhs) const noexcept = default;
};

void bindMaterial(const Material &material, ShaderProgram &program);
