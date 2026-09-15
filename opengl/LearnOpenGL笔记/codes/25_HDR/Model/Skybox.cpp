#include "Skybox.h"
#include "Helper.h"
#include "../Renderer/Renderer.h"
#include "../Buffers/InstanceBuffer.h"

Skybox::Skybox(const std::vector<std::string> &face_paths, bool flip_vertically)
    : mesh_{ createCubeMesh(InstanceBuffer{ InstanceBuffer::InstanceData{} }, false) }
    , cubemap_{ face_paths, flip_vertically }
{
    // 相机在立方体内部，看的是外侧面的背面 → 剔正面
    pipeline_state_.cull_face_ = true;
    pipeline_state_.cull_face_mode_ = GL_FRONT;
    pipeline_state_.depth_func_ = GL_LEQUAL;
    pipeline_state_.depth_write_ = false;
}

void Skybox::draw(ShaderProgram &shader) const
{
    pipeline_state_.apply();
    shader.use();
    cubemap_.bind(Renderer::SKYBOX_UNIT);
    mesh_.draw(static_cast<GLsizei>(mesh_.instanceCount()));
}
