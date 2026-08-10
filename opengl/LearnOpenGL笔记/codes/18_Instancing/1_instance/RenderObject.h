#pragma once

#include "Material.h"
#include "Mesh.h"
#include "ShaderProgram.h"
#include <memory>

class RenderObject {
public:
    Material material_;
    Mesh mesh_;

    void render(ShaderProgram *program = nullptr, std::optional<size_t> count = std::nullopt)
    {
        if (!program) {
            program = &material_.program_;
        }
        if (program) {
            program->use();
            material_.apply(*program);
        }
        mesh_.draw(count.has_value() ? count.value() : mesh_.instanceCount());
    }
};