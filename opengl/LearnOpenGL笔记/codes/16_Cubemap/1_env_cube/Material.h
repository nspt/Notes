#pragma once

#include "ShaderProgram.h"
#include "Texture.h"
#include <format>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Material {
public:
    std::shared_ptr<ShaderProgram> program_;
    std::vector<std::shared_ptr<Texture>> diffuse_textures_;
    std::vector<std::shared_ptr<Texture>> specular_textures_;
    std::shared_ptr<Texture> reflect_cube_texture_;
    std::shared_ptr<Texture> refract_cube_texture_;
    float refract_ratio_;
    float shininess_;
    bool pure_color_{ false };
    glm::vec3 color_{ 1.0f };

    void apply(ShaderProgram &program)
    {
        unsigned bp = 0;

        if (const GLint loc = program.uniformLocation("material.diffuse_count"); loc >= 0) {
            program.setInt(loc, static_cast<int>(diffuse_textures_.size()));
        }
        for (unsigned i = 0; i < diffuse_textures_.size(); ++i) {
            const auto name = std::format("material.diffuse_texture[{}]", i);
            if (const GLint loc = program.uniformLocation(name); loc >= 0) {
                diffuse_textures_[i]->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.specular_count"); loc >= 0) {
            program.setInt(loc, static_cast<int>(specular_textures_.size()));
        }
        for (unsigned i = 0; i < specular_textures_.size(); ++i) {
            const auto name = std::format("material.specular_texture[{}]", i);
            if (const GLint loc = program.uniformLocation(name); loc >= 0) {
                specular_textures_[i]->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.reflect_cube_exist"); loc >= 0) {
            program.setBool(loc, reflect_cube_texture_ ? true : false);
            if (reflect_cube_texture_) {
                if (const GLint tex_loc = program.uniformLocation("material.reflect_cube_texture"); tex_loc >= 0) {
                    reflect_cube_texture_->bind(0);
                    program.setInt(tex_loc, 0);
                }
            }
        }

        if (const GLint loc = program.uniformLocation("material.refract_cube_exist"); loc >= 0) {
            program.setBool(loc, refract_cube_texture_ ? true : false);
            if (refract_cube_texture_) {
                refract_cube_texture_->bind(0);
                if (const GLint tex_loc = program.uniformLocation("material.refract_cube_texture"); tex_loc >= 0) {
                    program.setInt(tex_loc, 0);
                }
                if (const GLint ratio_loc = program.uniformLocation("material.refract_ratio"); ratio_loc >= 0) {
                    program.setFloat(ratio_loc, refract_ratio_);
                }
            }
        }

        if (const GLint loc = program.uniformLocation("material.shininess"); loc >= 0) {
            program.setFloat(loc, shininess_);
        }

        if (const GLint loc = program.uniformLocation("material.pure_color"); loc >= 0) {
            program.setBool(loc, pure_color_);
        }

        if (const GLint loc = program.uniformLocation("material.color"); loc >= 0) {
            program.setVec3(loc, color_);
        }
    }
};
