#pragma once

#include "ShaderProgram.h"
#include "Texture2D.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Material {
public:
    std::shared_ptr<ShaderProgram> program_;
    std::shared_ptr<Texture2D> diffuse_map_;
    std::shared_ptr<Texture2D> specular_map_;
    float shininess_;

    void apply()
    {
        diffuse_map_->bind(0);
        specular_map_->bind(1);
        program_->setInt("material.diffuse_map", 0);
        program_->setInt("material.specular_map", 1);
        program_->setFloat("material.shininess", shininess_);
    }
};