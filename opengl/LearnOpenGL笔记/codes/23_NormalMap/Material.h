#pragma once

#include "ShaderProgram.h"
#include "Texture.h"
#include "Texture2D.h"
#include <format>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>

class Material {
public:
    std::vector<Texture> diffuse_textures_;
    std::vector<Texture> specular_textures_;
    std::vector<Texture> normal_textures_;
    float shininess_{ 32.0f };
    bool pure_color_{ false };
    glm::vec3 color_{ 1.0f };

    void apply(ShaderProgram &program, unsigned first_unit = 0)
    {
        unsigned bp = first_unit;

        // skybox.frag 用的是 uniform samplerCube skybox，不是 material.diffuse_texture
        if (const GLint loc = program.uniformLocation("skybox"); loc >= 0) {
            if (!diffuse_textures_.empty()) {
                diffuse_textures_[0].bind(bp);
                program.setInt(loc, static_cast<int>(bp));
            }
            return;
        }

        if (const GLint loc = program.uniformLocation("material.diffuse_count"); loc >= 0) {
            program.setInt(loc, static_cast<int>(diffuse_textures_.size()));
        }
        for (unsigned i = 0; i < diffuse_textures_.size(); ++i) {
            const auto name = std::format("material.diffuse_texture[{}]", i);
            if (const GLint loc = program.uniformLocation(name); loc >= 0) {
                diffuse_textures_[i].bind(bp);
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
                specular_textures_[i].bind(bp);
                program.setInt(loc, static_cast<int>(bp));
                ++bp;
            }
        }

        // 法线贴图必须始终绑定：无贴图时用平坦法线，避免未绑定 sampler / exist 标志失效
        if (const GLint loc = program.uniformLocation("material.normal_map"); loc >= 0) {
            if (!normal_textures_.empty()) {
                normal_textures_[0].bind(bp);
            } else {
                flatNormalMap().bind(bp);
            }
            program.setInt(loc, static_cast<int>(bp));
            ++bp;
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

private:
    // 切线空间平坦法线 (0,0,1) 编码为 RGB(128,128,255)
    static Texture2D &flatNormalMap()
    {
        static unsigned char pixel[] = { 128, 128, 255 };
        static Texture2D tex{ 1, 1, GL_RGB, GL_RGB, pixel };
        return tex;
    }
};
