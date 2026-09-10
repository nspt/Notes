#include "PipelineState.h"

namespace {

bool g_cache_valid{ false };
PipelineState g_prev_state{};

} // namespace

void PipelineState::setCapability(GLenum cap, bool enabled)
{
    if (enabled) {
        glEnable(cap);
    } else {
        glDisable(cap);
    }
}

void PipelineState::invalidate()
{
    g_cache_valid = false;
}

void PipelineState::apply() const
{
    if (g_cache_valid && g_prev_state == *this) {
        return;
    }

    setCapability(GL_DEPTH_TEST, depth_test_);
    setCapability(GL_BLEND, blend_);
    setCapability(GL_STENCIL_TEST, stencil_test_);
    setCapability(GL_CULL_FACE, cull_face_);

    glDepthFunc(depth_func_);
    glCullFace(cull_face_mode_);

    glStencilFunc(stencil_func_, stencil_ref_, stencil_mask_);
    glStencilOp(stencil_sfail_, stencil_dpfail_, stencil_dppass_);
    glStencilMask(stencil_write_mask_);

    glBlendFuncSeparate(blend_src_rgb_, blend_dst_rgb_, blend_src_alpha_, blend_dst_alpha_);

    g_prev_state = *this;
    g_cache_valid = true;
}
