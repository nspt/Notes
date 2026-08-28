#pragma once

#include <glad/glad.h>

class RenderState {
public:
    bool depth_test_{ true };
    bool blend_{ false };
    bool stencil_test_{ false };
    bool cull_face_{ true };

    GLenum depth_func_{ GL_LESS };
    GLenum cull_face_mode_{ GL_BACK };

    GLenum stencil_func_{ GL_ALWAYS };
    GLint stencil_ref_{ 0 };
    GLuint stencil_mask_{ 0xff };
    GLenum stencil_sfail_{ GL_KEEP };
    GLenum stencil_dpfail_{ GL_KEEP };
    GLenum stencil_dppass_{ GL_KEEP };
    GLuint stencil_write_mask_{ 0xff };

    GLenum blend_src_rgb_{ GL_SRC_ALPHA };
    GLenum blend_dst_rgb_{ GL_ONE_MINUS_SRC_ALPHA };
    GLenum blend_src_alpha_{ GL_ONE };
    GLenum blend_dst_alpha_{ GL_ZERO };

    friend bool operator==(const RenderState &a, const RenderState &b) noexcept = default;
    friend bool operator!=(const RenderState &a, const RenderState &b) noexcept = default;

    void apply() const
    {
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
    }

    void applyIfChanged(RenderState &cached) const
    {
        if (*this == cached) {
            return;
        }
        apply();
        cached = *this;
    }

private:
    static void setCapability(GLenum cap, bool enabled)
    {
        if (enabled) {
            glEnable(cap);
        } else {
            glDisable(cap);
        }
    }
};
