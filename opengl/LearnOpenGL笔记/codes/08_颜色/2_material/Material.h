#pragma once

#include "ShaderProgram.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Material {
public:
    std::shared_ptr<ShaderProgram> program_;
    glm::vec3 ambient_;
    glm::vec3 diffuse_;
    glm::vec3 specular_;
    float shininess_;

    void apply()
    {
        program_->setVec3("material.ambient", ambient_);
        program_->setVec3("material.diffuse", diffuse_);
        program_->setVec3("material.specular", specular_);
        program_->setFloat("material.shininess", shininess_);
    }
};