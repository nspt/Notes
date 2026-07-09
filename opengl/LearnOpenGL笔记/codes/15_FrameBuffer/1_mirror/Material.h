#pragma once

#include "ShaderProgram.h"
#include "Texture2D.h"
#include <format>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Material {
public:
    std::shared_ptr<ShaderProgram> program_;
    std::vector<std::shared_ptr<Texture2D>> diffuse_textures_;
    std::vector<std::shared_ptr<Texture2D>> specular_textures_;
    float shininess_;
    bool pure_color_{ false };
    glm::vec3 color_{ 1.0f };

    void apply(ShaderProgram &program)
    {
        unsigned bp = 0;

        program.setInt("material.diffuse_count", diffuse_textures_.size());
        for (unsigned i = 0; i < diffuse_textures_.size(); ++i) {
            diffuse_textures_[i]->bind(bp);
            program.setInt(std::format("material.diffuse_texture[{}]", i), bp);
            ++bp;
        }

        program.setInt("material.specular_count", specular_textures_.size());
        for (unsigned i = 0; i < specular_textures_.size(); ++i) {
            specular_textures_[i]->bind(bp);
            program.setInt(std::format("material.specular_texture[{}]", i), bp);
            ++bp;
        }

        program.setFloat("material.shininess", shininess_);
        program.setBool("material.pure_color", pure_color_);
        program.setVec3("material.color", color_);
    }
};