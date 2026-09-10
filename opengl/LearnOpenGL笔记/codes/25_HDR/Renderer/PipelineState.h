#pragma once

#include <glad/glad.h>

class PipelineState {
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

    friend bool operator==(const PipelineState &a, const PipelineState &b) noexcept = default;
    friend bool operator!=(const PipelineState &a, const PipelineState &b) noexcept = default;

    void apply() const;
    static void invalidate();

private:
    static void setCapability(GLenum cap, bool enabled);
};
