#pragma once

#include "Material.h"
#include "Mesh.h"
#include "../Renderer/PipelineState.h"
#include "../Renderer/ShaderProgram.h"

class RenderObject {
public:
    Material material_;
    Mesh mesh_;
    ShaderProgram render_shader_;
    ShaderProgram directional_shadow_shader_; // 方向光 / 聚光（2D shadow map）
    ShaderProgram omni_shadow_shader_;        // 点光源 / 万向光（cubemap + GS）
    PipelineState pipeline_state_;
};
