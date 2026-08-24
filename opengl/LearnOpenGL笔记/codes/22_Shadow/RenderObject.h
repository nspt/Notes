#pragma once

#include "Material.h"
#include "Mesh.h"
#include "RenderState.h"
#include "ShaderProgram.h"
#include <memory>
#include <optional>

class RenderObject {
public:
    Material material_;
    Mesh mesh_;
    ShaderProgram render_shader_;
    ShaderProgram shadow_shader_;
    RenderState render_state_;

    void render(ShaderProgram *program = nullptr, std::optional<size_t> count = std::nullopt)
    {
        if (!program) {
            program = &render_shader_;
        }
        if (program->id() == 0) {
            return;
        }
        applyRenderState();
        program->use();
        material_.apply(*program);
        mesh_.draw(count.has_value() ? count.value() : mesh_.instanceCount());
    }

    void renderShadow(ShaderProgram *program = nullptr, std::optional<size_t> count = std::nullopt)
    {
        if (!program) {
            program = &shadow_shader_;
        }
        if (program->id() == 0) {
            return;
        }
        program->use();
        material_.apply(*program);
        mesh_.draw(count.has_value() ? count.value() : mesh_.instanceCount());
    }

    // 外部直接改了 GL 状态后调用，避免缓存与真实状态不一致
    static void invalidateCachedState() noexcept
    {
        s_cache_valid_ = false;
    }

private:
    void applyRenderState() const
    {
        if (!s_cache_valid_ || render_state_ != s_cached_state_) {
            render_state_.apply();
            s_cached_state_ = render_state_;
            s_cache_valid_ = true;
        }
    }

    static inline bool s_cache_valid_{ false };
    static inline RenderState s_cached_state_{};
};
