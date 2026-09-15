#include "ScreenQuad.h"
#include "Helper.h"
#include "../Buffers/InstanceBuffer.h"
#include "../Renderer/Renderer.h"

ScreenQuad::ScreenQuad()
    : mesh_{ createQuadMesh(InstanceBuffer{ InstanceBuffer::InstanceData{} }, false) }
{
    pipeline_state_.depth_test_ = false;
    pipeline_state_.cull_face_ = false;
}

void ScreenQuad::draw(ShaderProgram &shader) const
{
    pipeline_state_.apply();
    shader.use();
    if (texture_) {
        texture_->bind(Renderer::QUAD_TEXTURE_UNIT);
    }
    mesh_.draw(static_cast<GLsizei>(mesh_.instanceCount()));
}
