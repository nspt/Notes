#include "Material.h"
#include "../Renderer/Renderer.h"

void bindMaterial(const Material &material, ShaderProgram &program)
{
    program.setUniformIfPresent("material.diffuse_exist", material.diffuse_.has_value());
    if (material.diffuse_) {
        material.diffuse_->bind(Renderer::DIFFUSE_UNIT);
    }

    program.setUniformIfPresent("material.specular_exist", material.specular_.has_value());
    if (material.specular_) {
        material.specular_->bind(Renderer::SPECULAR_UNIT);
    }

    program.setUniformIfPresent("material.normal_exist", material.normal_.has_value());
    if (material.normal_) {
        material.normal_->bind(Renderer::NORMAL_UNIT);
    }

    program.setUniformIfPresent("material.height_exist", material.height_.has_value());
    if (material.height_) {
        material.height_->bind(Renderer::HEIGHT_UNIT);
    }
    program.setUniformIfPresent("material.height_scale", material.height_scale_);
    program.setUniformIfPresent("material.shininess", material.shininess_);
    program.setUniformIfPresent("material.pure_color", material.pure_color_);
    program.setUniformIfPresent("material.color", material.color_);
    program.setUniformIfPresent("material.reflect_cube_exist", false);
    program.setUniformIfPresent("material.refract_cube_exist", false);
}
