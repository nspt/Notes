#pragma once

#include "../Renderer/PipelineState.h"
#include "../Renderer/ShaderProgram.h"
#include "../Textures/Texture.h"
#include "Mesh.h"
#include <optional>

class ScreenQuad {
public:
    ScreenQuad();

    void draw(ShaderProgram &shader) const;

    Mesh mesh_;
    std::optional<Texture> texture_;
    PipelineState pipeline_state_;
};
