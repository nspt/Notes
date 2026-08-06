#pragma once

#include "ShaderProgram.h"
#include "Texture.h"
#include <format>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Material {
public:

private:
    struct Data {
        ShaderProgram program_;
        std::vector<Texture> diffuse_textures_;
        std::vector<Texture> specular_textures_;
        float shininess_;
        bool pure_color_{ false };
        glm::vec3 color_{ 1.0f };
    };
    std::shared_ptr<Data> data_{
        std::make_shared<Data>()
    };

    ShaderProgram &program() { return data_->program_; }
    std::vector<Texture> &diffuseTextures() { return data_->diffuse_textures_; }
    std::vector<Texture> &specularTextures() { return data_->specular_textures_; }
    float &shininess() { return data_->shininess_; }
    bool &pureColor() { return data_->pure_color_; }
    glm::vec3 &color() { return data_->color_; }

    void apply(ShaderProgram &program)
    {
        unsigned bp = 0;

        if (const GLint loc = program.uniformLocation("material.diffuse_count"); loc >= 0) {
            program.setInt(loc, static_cast<int>(data_->diffuse_textures_.size()));
        }
        for (unsigned i = 0; i < data_->diffuse_textures_.size(); ++i) {
            const auto name = std::format("material.diffuse_texture[{}]", i);
            if (const GLint loc = program.uniformLocation(name); loc >= 0) {
                data_->diffuse_textures_[i].bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.specular_count"); loc >= 0) {
            program.setInt(loc, static_cast<int>(data_->specular_textures_.size()));
        }
        for (unsigned i = 0; i < data_->specular_textures_.size(); ++i) {
            const auto name = std::format("material.specular_texture[{}]", i);
            if (const GLint loc = program.uniformLocation(name); loc >= 0) {
                data_->specular_textures_[i].bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.shininess"); loc >= 0) {
            program.setFloat(loc, data_->shininess_);
        }
        if (const GLint loc = program.uniformLocation("material.pure_color"); loc >= 0) {
            program.setBool(loc, data_->pure_color_);
        }
        if (const GLint loc = program.uniformLocation("material.color"); loc >= 0) {
            program.setVec3(loc, data_->color_);
        }
    }
};
