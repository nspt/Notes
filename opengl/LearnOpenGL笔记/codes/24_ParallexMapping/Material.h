#pragma once

#include "ShaderProgram.h"
#include "Texture.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <optional>

class Material {
public:
    std::optional<Texture> diffuse_;
    std::optional<Texture> specular_;
    std::optional<Texture> normal_;
    std::optional<Texture> height_;
    std::optional<Texture> skybox_;
    float height_scale_{ 0.1f };
    float shininess_{ 32.0f };
    bool pure_color_{ false };
    glm::vec3 color_{ 1.0f };

    void apply(ShaderProgram &program, unsigned first_unit = 0)
    {
        unsigned bp = first_unit;

        if (const GLint loc = program.uniformLocation("skybox"); loc >= 0) {
            if (skybox_) {
                skybox_->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
            }
            return;
        }

        if (const GLint loc = program.uniformLocation("material.diffuse_exist"); loc >= 0) {
            program.setBool(loc, diffuse_.has_value());
        }
        if (diffuse_) {
            if (const GLint loc = program.uniformLocation("material.diffuse_texture"); loc >= 0) {
                diffuse_->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.specular_exist"); loc >= 0) {
            program.setBool(loc, specular_.has_value());
        }
        if (specular_) {
            if (const GLint loc = program.uniformLocation("material.specular_texture"); loc >= 0) {
                specular_->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.normal_exist"); loc >= 0) {
            program.setBool(loc, normal_.has_value());
        }
        if (normal_) {
            if (const GLint loc = program.uniformLocation("material.normal_map"); loc >= 0) {
                normal_->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        if (const GLint loc = program.uniformLocation("material.height_exist"); loc >= 0) {
            program.setBool(loc, height_.has_value());
        }
        if (height_) {
            if (const GLint loc = program.uniformLocation("material.height_map"); loc >= 0) {
                height_->bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }
        if (const GLint loc = program.uniformLocation("material.height_scale"); loc >= 0) {
            program.setFloat(loc, height_scale_);
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
        // 本课未用环境反射/折射；显式关闭，避免 AMD 上未绑定 samplerCube 与 unit0 冲突
        if (const GLint loc = program.uniformLocation("material.reflect_cube_exist"); loc >= 0) {
            program.setBool(loc, false);
        }
        if (const GLint loc = program.uniformLocation("material.refract_cube_exist"); loc >= 0) {
            program.setBool(loc, false);
        }
    }
};
