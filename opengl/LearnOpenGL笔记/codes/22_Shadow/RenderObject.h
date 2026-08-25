#pragma once

#include "Material.h"
#include "Mesh.h"
#include "RenderState.h"
#include "ShaderProgram.h"
#include <functional>
#include <memory>
#include <optional>

enum class RenderPass {
    Draw,
    Shadow
};

class RenderObject {
public:
    using Action = std::function<void(ShaderProgram &shader, RenderPass pass)>;

    Material material_;
    Mesh mesh_;
    ShaderProgram render_shader_;
    ShaderProgram directional_shadow_shader_; // 方向光 / 聚光（2D shadow map）
    ShaderProgram omni_shadow_shader_;        // 点光源 / 万向光（cubemap + GS）
    RenderState render_state_;
    std::optional<Action> action_;

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
        if (action_) {
            (*action_)(*program, RenderPass::Draw);
        }
        material_.apply(*program);
        mesh_.draw(count.has_value() ? count.value() : mesh_.instanceCount());
    }

    void renderDirectionalShadow(std::optional<size_t> count = std::nullopt)
    {
        renderShadowWith(directional_shadow_shader_, count);
    }

    void renderOmniShadow(std::optional<size_t> count = std::nullopt)
    {
        renderShadowWith(omni_shadow_shader_, count);
    }

    // 外部直接改了 GL 状态后调用，避免缓存与真实状态不一致
    static void invalidateCachedState() noexcept
    {
        s_cache_valid_ = false;
    }

private:
    void renderShadowWith(ShaderProgram &shader, std::optional<size_t> count)
    {
        if (shader.id() == 0) {
            return;
        }
        shader.use();
        if (action_) {
            (*action_)(shader, RenderPass::Shadow);
        }
        material_.apply(shader);
        mesh_.draw(count.has_value() ? count.value() : mesh_.instanceCount());
    }

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
