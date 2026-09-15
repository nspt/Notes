#pragma once

#include "../Renderer/PipelineState.h"
#include "../Renderer/ShaderProgram.h"
#include "../Textures/TextureCubeMap.h"
#include "Mesh.h"
#include <string>
#include <vector>

class Skybox {
public:
    explicit Skybox(const std::vector<std::string> &face_paths, bool flip_vertically = false);

    void draw(ShaderProgram &shader) const;

    Mesh mesh_;
    TextureCubeMap cubemap_;
    PipelineState pipeline_state_;
};
