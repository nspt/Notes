#include "Renderer.h"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <variant>

#include "../FrameBuffer/RenderBufferMS.h"
#include "../Scene/Helper.h"
#include "../Textures/Texture2D.h"
#include "../Textures/Texture2DMS.h"

Renderer::ShaderMap Renderer::shaders_{};

namespace {

template <typename T>
T getAttachmentOrThrow(const FrameBuffer &fb, GLenum attachment)
{
    auto opt = fb.attachment(attachment);
    if (!opt) {
        throw std::runtime_error{ "FrameBuffer attachment missing" };
    }
    if (!std::holds_alternative<T>(*opt)) {
        throw std::runtime_error{ "FrameBuffer attachment type mismatch" };
    }
    return std::get<T>(*opt);
}

} // namespace

Renderer::Renderer()
{
    createFrameBuffers();
    createShadowResources();
}

Renderer::~Renderer() {}

void Renderer::setScene(Scene scene)
{
    scene_ = std::move(scene);
    light_ubo_.setSubData(0, sizeof(LightData), &scene_.data_->lights_);
    reallocFrameBuffers();
    createShadowResources();
}

Scene Renderer::scene() const
{
    return scene_;
}

void Renderer::setCameraData(const CameraData &cam)
{
    camera_data_ = cam;
    cam_data_ubo_.setSubData(0, sizeof(camera_data_), &camera_data_);
}

CameraData Renderer::cameraData() const
{
    return camera_data_;
}

void Renderer::setShaders(ShaderMap shaders)
{
    shaders_ = std::move(shaders);
}

Renderer::ShaderMap Renderer::allShaders() const
{
    return shaders_;
}

ShaderProgram Renderer::shader(const std::string &name) const
{
    return shaders_.at(name);
}

void Renderer::setFrameBuffer(FrameBuffer framebuffer)
{
    final_framebuffer_ = std::move(framebuffer);
}

FrameBuffer Renderer::frameBuffer() const
{
    return final_framebuffer_.value();
}

void Renderer::resetFrameBuffer()
{
    final_framebuffer_.reset();
}

void Renderer::setViewport(int x, int y, int width, int height)
{
    const int old_w = std::get<2>(viewport_);
    const int old_h = std::get<3>(viewport_);
    std::get<0>(viewport_) = x;
    std::get<1>(viewport_) = y;
    std::get<2>(viewport_) = std::max(width, 1);
    std::get<3>(viewport_) = std::max(height, 1);
    if (std::get<2>(viewport_) != old_w || std::get<3>(viewport_) != old_h) {
        reallocFrameBuffers();
    }
}

std::tuple<int, int, int, int> Renderer::viewport() const
{
    return viewport_;
}

void Renderer::setPostProcParams(PostProcParams params)
{
    post_proc_params_ = std::move(params);
}

Renderer::PostProcParams Renderer::postProcParams() const
{
    return post_proc_params_;
}

void Renderer::setPostProcShader(ShaderProgram shader)
{
    post_proc_shader_ = std::move(shader);
}

void Renderer::setBlurShader(ShaderProgram shader)
{
    blur_shader_ = std::move(shader);
}

void Renderer::setCompositeShader(ShaderProgram shader)
{
    composite_shader_ = std::move(shader);
}

void Renderer::setBloomThreshold(float threshold)
{
    bloom_threshold_ = threshold;
}

float Renderer::bloomThreshold() const
{
    return bloom_threshold_;
}

void Renderer::setHdrEnabled(bool enabled)
{
    if (hdr_enabled_ == enabled) {
        return;
    }
    hdr_enabled_ = enabled;
    reallocFrameBuffers();
}

bool Renderer::hdrEnabled() const
{
    return hdr_enabled_;
}

void Renderer::setBloomEnabled(bool enabled)
{
    bloom_enabled_ = enabled;
}

bool Renderer::bloomEnabled() const
{
    return bloom_enabled_;
}

GLenum Renderer::colorInternalFormat() const noexcept
{
    return hdr_enabled_ ? GL_RGB16F : GL_RGB;
}

void Renderer::render()
{
    bindUniformBuffers();
    shadowPass();
    forwardPass();
    if (bloom_enabled_) {
        blurBrightPass(std::get<2>(viewport_), std::get<3>(viewport_));
        compositePass();
    }
    postProcPass();
}

void Renderer::createFrameBuffers()
{
    const int width = std::get<2>(viewport_);
    const int height = std::get<3>(viewport_);
    const GLenum color_fmt = colorInternalFormat();

    Texture2DMS color_ms{ width, height, color_fmt, hdr_samples_ };
    Texture2DMS bright_ms{ width, height, color_fmt, hdr_samples_ };
    RenderBufferMS depth_stencil{ GL_DEPTH24_STENCIL8, width, height, hdr_samples_ };
    hdr_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT0, color_ms);
    hdr_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT1, bright_ms);
    hdr_framebuffer_.attachRBO(GL_DEPTH_STENCIL_ATTACHMENT, depth_stencil);
    const GLenum hdr_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    hdr_framebuffer_.drawBuffers(2, hdr_bufs);
    if (!hdr_framebuffer_.isCompleted()) {
        throw std::runtime_error{ "HDR MSAA FBO is not completed" };
    }

    Texture2D color{ width, height, color_fmt, GL_RGB, nullptr };
    Texture2D bright{ width, height, color_fmt, GL_RGB, nullptr };
    color.setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    bright.setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    resolve_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT0, color);
    resolve_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT1, bright);
    const GLenum resolve_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    resolve_framebuffer_.drawBuffers(2, resolve_bufs);
    if (!resolve_framebuffer_.isCompleted()) {
        throw std::runtime_error{ "HDR resolve FBO is not completed" };
    }

    Texture2D blur_color{ width, height, color_fmt, GL_RGB, nullptr };
    blur_color.setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    blur_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT0, blur_color);
    const GLenum blur_bufs[] = { GL_COLOR_ATTACHMENT0 };
    blur_framebuffer_.drawBuffers(1, blur_bufs);
    if (!blur_framebuffer_.isCompleted()) {
        throw std::runtime_error{ "Bloom blur FBO is not completed" };
    }

    Texture2D composite_color{ width, height, color_fmt, GL_RGB, nullptr };
    composite_color.setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    composite_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT0, composite_color);
    composite_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT0);
    if (!composite_framebuffer_.isCompleted()) {
        throw std::runtime_error{ "Composite FBO is not completed" };
    }

    post_proc_quad_.mesh_ = createQuadMesh(
        InstanceBuffer{ InstanceBuffer::InstanceData{} }, false
    );
    post_proc_quad_.pipeline_state_.depth_test_ = false;
    post_proc_quad_.pipeline_state_.cull_face_ = false;

    FrameBuffer::unbind();
}

void Renderer::reallocFrameBuffers()
{
    const int width = std::max(std::get<2>(viewport_), 1);
    const int height = std::max(std::get<3>(viewport_), 1);
    const GLenum color_fmt = colorInternalFormat();

    auto color_ms = getAttachmentOrThrow<Texture2DMS>(hdr_framebuffer_, GL_COLOR_ATTACHMENT0);
    auto bright_ms = getAttachmentOrThrow<Texture2DMS>(hdr_framebuffer_, GL_COLOR_ATTACHMENT1);
    auto depth_stencil = getAttachmentOrThrow<RenderBufferMS>(
        hdr_framebuffer_, GL_DEPTH_STENCIL_ATTACHMENT
    );
    if (color_ms.width() != width || color_ms.height() != height || color_ms.format() != color_fmt) {
        color_ms.reallocate(width, height, color_fmt, hdr_samples_);
        bright_ms.reallocate(width, height, color_fmt, hdr_samples_);
        depth_stencil.reallocate(GL_DEPTH24_STENCIL8, width, height, hdr_samples_);
    }

    auto color = getAttachmentOrThrow<Texture2D>(resolve_framebuffer_, GL_COLOR_ATTACHMENT0);
    auto bright = getAttachmentOrThrow<Texture2D>(resolve_framebuffer_, GL_COLOR_ATTACHMENT1);
    if (color.width() != width || color.height() != height || color.format() != color_fmt) {
        color.reallocate(width, height, color_fmt, GL_RGB, nullptr);
        bright.reallocate(width, height, color_fmt, GL_RGB, nullptr);
    }

    auto blur_color = getAttachmentOrThrow<Texture2D>(blur_framebuffer_, GL_COLOR_ATTACHMENT0);
    if (blur_color.width() != width || blur_color.height() != height || blur_color.format() != color_fmt) {
        blur_color.reallocate(width, height, color_fmt, GL_RGB, nullptr);
    }

    auto composite_color = getAttachmentOrThrow<Texture2D>(
        composite_framebuffer_, GL_COLOR_ATTACHMENT0
    );
    if (composite_color.width() != width || composite_color.height() != height
        || composite_color.format() != color_fmt) {
        composite_color.reallocate(width, height, color_fmt, GL_RGB, nullptr);
    }
}

void Renderer::bindFinalFrameBuffer() const
{
    if (final_framebuffer_) {
        final_framebuffer_->bind();
    } else {
        FrameBuffer::unbind();
    }
}

void Renderer::createShadowResources(int shadow_map_size)
{
    auto &sd{ scene_.data_->shadows_ };
    const auto directional_count = static_cast<size_t>(sd.directional_.size());
    const auto point_count = static_cast<size_t>(sd.point_.size());
    const auto spot_count = static_cast<size_t>(sd.spot_.size());

    auto &sr{ shadow_resources_ };
    sr = {};
    for (size_t i = 0; i < directional_count; ++i) {
        auto texture = createShadowMapTexture(shadow_map_size);
        sr.directional_.push_back({
            sd.directional_[i], texture, createShadowMapFBO(texture)
        });
    }
    for (size_t i = 0; i < spot_count; ++i) {
        auto texture = createShadowMapTexture(shadow_map_size);
        sr.spot_.push_back({
            sd.spot_[i], texture, createShadowMapFBO(texture)
        });
    }
    for (size_t i = 0; i < point_count; ++i) {
        auto texture = createShadowMapTextureCube(shadow_map_size);
        sr.point_.push_back({
            sd.point_[i], texture, createShadowMapFBO(texture)
        });
    }
}

void Renderer::shadowPass()
{
    ShadowResources &sr{ shadow_resources_ };
    PipelineState shadow_state;
    shadow_state.cull_face_mode_ = GL_FRONT;
    shadow_state.apply();

    for (size_t i = 0; i < sr.directional_.size(); ++i) {
        sr.directional_[i].fb.bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, sr.directional_[i].texture.width(), sr.directional_[i].texture.height());
        renderDirShadow(sr.directional_[i].params.transform);
    }
    for (size_t i = 0; i < sr.spot_.size(); ++i) {
        sr.spot_[i].fb.bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, sr.spot_[i].texture.width(), sr.spot_[i].texture.height());
        renderDirShadow(sr.spot_[i].params.transform);
    }
    for (size_t i = 0; i < sr.point_.size(); ++i) {
        const auto &params = sr.point_[i].params;
        sr.point_[i].fb.bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, sr.point_[i].texture.width(), sr.point_[i].texture.height());
        renderOmniShadow(
            params.transforms,
            glm::vec3(params.position),
            params.far_plane
        );
    }
    glCullFace(GL_BACK);
    PipelineState::invalidate();
}

void Renderer::blitColorAttachment(
    FrameBuffer &src, FrameBuffer *dst,
    GLenum src_attachment, GLenum dst_attachment,
    int src_w, int src_h,
    int dst_x, int dst_y, int dst_w, int dst_h)
{
    src.bind(GL_READ_FRAMEBUFFER);
    src.readBuffer(src_attachment);
    if (dst) {
        dst->bind(GL_DRAW_FRAMEBUFFER);
        dst->drawBuffer(dst_attachment);
    } else {
        FrameBuffer::unbind(GL_DRAW_FRAMEBUFFER);
        glDrawBuffer(GL_BACK);
    }
    glBlitFramebuffer(
        0, 0, src_w, src_h,
        dst_x, dst_y, dst_x + dst_w, dst_y + dst_h,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
}

void Renderer::drawFullscreenQuad(ShaderProgram &program)
{
    post_proc_quad_.pipeline_state_.apply();
    program.use();
    post_proc_quad_.mesh_.draw(static_cast<GLsizei>(post_proc_quad_.mesh_.instanceCount()));
}

void Renderer::blurBrightPass(int width, int height)
{
    if (!blur_shader_) {
        return;
    }

    auto bright_color = getAttachmentOrThrow<Texture2D>(
        resolve_framebuffer_, GL_COLOR_ATTACHMENT1
    );
    auto blur_color = getAttachmentOrThrow<Texture2D>(
        blur_framebuffer_, GL_COLOR_ATTACHMENT0
    );

    auto &blur_shader = blur_shader_.value();
    blur_shader.setVec2(
        "tex_offset",
        glm::vec2(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height))
    );
    blur_shader.setInt("image", 0);

    bool horizontal = true;
    for (int i = 0; i < bloom_blur_iterations_ * 2; ++i) {
        // horizontal → blur FBO（读 resolve att1）；vertical → resolve att1（读 blur FBO）
        if (horizontal) {
            blur_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT0);
            bright_color.bind(0);
        } else {
            resolve_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT1);
            blur_color.bind(0);
        }
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        blur_shader.setBool("horizontal", horizontal);
        drawFullscreenQuad(blur_shader);
        horizontal = !horizontal;
    }

    const GLenum resolve_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    resolve_framebuffer_.drawBuffers(2, resolve_bufs);
}

void Renderer::forwardPass()
{
    reallocFrameBuffers();

    const int width = std::get<2>(viewport_);
    const int height = std::get<3>(viewport_);

    if (bloom_enabled_) {
        const GLenum hdr_bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        hdr_framebuffer_.drawBuffers(2, hdr_bufs);
    } else {
        hdr_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT0);
    }
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    applySceneShadows();
    if (bloom_enabled_) {
        applyBloomThreshold();
    }

    std::optional<GLuint> prev_shader_id;
    auto draw_model = [&](Model &model) {
        for (auto &object : model.objects_) {
            auto &program = object.render_shader_;
            if (program.id() == 0) {
                continue;
            }
            object.pipeline_state_.apply();
            if (!prev_shader_id || program.id() != *prev_shader_id) {
                program.use();
                prev_shader_id = program.id();
            }
            applyMaterial(object.material_, program);
            object.mesh_.draw(static_cast<GLsizei>(object.mesh_.instanceCount()));
        }
    };

    for (auto &model : scene_.data_->models_) {
        draw_model(model);
    }
    for (auto &[model, distance] : sortObjectsByDistance(scene_.data_->transparent_models_)) {
        (void)distance;
        draw_model(*model);
    }

    if (bloom_enabled_) {
        blitColorAttachment(
            hdr_framebuffer_, &resolve_framebuffer_,
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0,
            width, height, 0, 0, width, height
        );
        blitColorAttachment(
            hdr_framebuffer_, &resolve_framebuffer_,
            GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT1,
            width, height, 0, 0, width, height
        );
    } else {
        blitColorAttachment(
            hdr_framebuffer_, &composite_framebuffer_,
            GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0,
            width, height, 0, 0, width, height
        );
    }
}

void Renderer::compositePass()
{
    const int width = std::get<2>(viewport_);
    const int height = std::get<3>(viewport_);

    auto scene_color = getAttachmentOrThrow<Texture2D>(
        resolve_framebuffer_, GL_COLOR_ATTACHMENT0
    );
    auto bloom_color = getAttachmentOrThrow<Texture2D>(
        resolve_framebuffer_, GL_COLOR_ATTACHMENT1
    );
    auto &composite_shader = composite_shader_.value();

    composite_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    scene_color.bind(0);
    bloom_color.bind(1);
    composite_shader.setInt("scene", 0);
    composite_shader.setInt("bloomBlur", 1);
    drawFullscreenQuad(composite_shader);
    FrameBuffer::unbind();
}

void Renderer::postProcPass()
{
    const int x = std::get<0>(viewport_);
    const int y = std::get<1>(viewport_);
    const int width = std::get<2>(viewport_);
    const int height = std::get<3>(viewport_);

    bindFinalFrameBuffer();
    glViewport(x, y, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto composite_color = getAttachmentOrThrow<Texture2D>(
        composite_framebuffer_, GL_COLOR_ATTACHMENT0
    );
    auto &quad_shader = post_proc_shader_.value();

    composite_color.bind(0);
    quad_shader.setInt("quad_texture", 0);
    quad_shader.setVec2(
        "tex_offset",
        glm::vec2(1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height))
    );
    quad_shader.setBool("enable_kernel", post_proc_params_.enable_kernel);
    if (post_proc_params_.enable_kernel) {
        quad_shader.setFLoatArr("kernel", post_proc_params_.kernel, 9);
    }
    quad_shader.setFloat("exposure", post_proc_params_.exposure);
    quad_shader.setFloat("gamma", post_proc_params_.gamma);
    quad_shader.setBool("enable_tone_mapping", hdr_enabled_);
    drawFullscreenQuad(quad_shader);
    FrameBuffer::unbind();
}

void Renderer::renderDirShadow(const glm::mat4 &light_space)
{
    std::optional<GLuint> prev_shader_id;
    auto draw_shadow_models = [&](std::vector<Model> &models) {
        for (auto &model : models) {
            for (auto &object : model.objects_) {
                auto &shader = object.directional_shadow_shader_;
                if (shader.id() == 0)
                    continue;
                if (!prev_shader_id || shader.id() != *prev_shader_id) {
                    shader.setMat4("light_space_transform", light_space);
                    shader.use();
                    prev_shader_id = shader.id();
                }
                object.mesh_.draw(static_cast<GLsizei>(object.mesh_.instanceCount()));
            }
        }
    };
    draw_shadow_models(scene_.data_->models_);
    draw_shadow_models(scene_.data_->transparent_models_);
}

void Renderer::renderOmniShadow(const glm::mat4 *light_spaces,
                               const glm::vec3 &light_pos,
                               float far_plane)
{
    std::optional<GLuint> prev_shader_id;
    auto draw_shadow_models = [&](std::vector<Model> &models) {
        for (auto &model : models) {
            for (auto &object : model.objects_) {
                auto &shader = object.omni_shadow_shader_;
                if (shader.id() == 0)
                    continue;
                if (!prev_shader_id || shader.id() != *prev_shader_id) {
                    shader.setMat4Arr("light_space_transform", light_spaces, 6);
                    shader.setVec3("light_pos", light_pos);
                    shader.setFloat("far_plane", far_plane);
                    shader.use();
                    prev_shader_id = shader.id();
                }
                object.mesh_.draw(static_cast<GLsizei>(object.mesh_.instanceCount()));
            }
        }
    };
    draw_shadow_models(scene_.data_->models_);
    draw_shadow_models(scene_.data_->transparent_models_);
}

void Renderer::bindUniformBuffers() const
{
    light_ubo_.bindBase(0);
    cam_data_ubo_.bindBase(1);
}

void Renderer::applySceneShadows()
{
    std::unordered_set<GLuint> applied_shader_ids;
    auto apply_for_models = [&](std::vector<Model> &models) {
        for (auto &model : models) {
            for (auto &object : model.objects_) {
                auto &program = object.render_shader_;
                if (program.id() == 0) {
                    continue;
                }
                if (applied_shader_ids.insert(program.id()).second) {
                    applyShadow(shadow_resources_, program);
                }
            }
        }
    };
    apply_for_models(scene_.data_->models_);
    apply_for_models(scene_.data_->transparent_models_);
}

void Renderer::applyBloomThreshold()
{
    std::unordered_set<GLuint> applied_shader_ids;
    auto apply_for_models = [&](std::vector<Model> &models) {
        for (auto &model : models) {
            for (auto &object : model.objects_) {
                auto &program = object.render_shader_;
                if (program.id() == 0) {
                    continue;
                }
                if (applied_shader_ids.insert(program.id()).second) {
                    program.setUniformIfPresent("bloom_threshold", bloom_threshold_);
                }
            }
        }
    };
    apply_for_models(scene_.data_->models_);
    apply_for_models(scene_.data_->transparent_models_);
}

void Renderer::applyShadow(const ShadowResources &shadow, ShaderProgram &shader, unsigned first_unit)
{
    shader.setUniformIfPresent("shadowMap.counts", glm::ivec4{
        static_cast<int>(shadow.directional_.size()),
        static_cast<int>(shadow.point_.size()),
        static_cast<int>(shadow.spot_.size()),
        0
    });

    unsigned unit = first_unit;
    for (size_t i = 0; i < shadow.directional_.size(); ++i, ++unit) {
        const auto &res = shadow.directional_[i];
        const float texel = 1.0f / static_cast<float>(res.texture.width());
        res.texture.bind(unit);
        shader.setUniformIfPresent(std::format("shadowMap.directional[{}].map", i),
                                   static_cast<int>(unit));
        shader.setUniformIfPresent(std::format("shadowMap.directional[{}].transform", i),
                                   res.params.transform);
        shader.setUniformIfPresent(std::format("shadowMap.directional[{}].bias.min_bias", i),
                                   res.params.bias.min_bias);
        shader.setUniformIfPresent(std::format("shadowMap.directional[{}].bias.slope_bias", i),
                                   res.params.bias.slope_bias);
        shader.setUniformIfPresent(std::format("shadowMap.directional[{}].texel_size", i),
                                   glm::vec2{ texel, texel });
    }
    for (size_t i = 0; i < shadow.spot_.size(); ++i, ++unit) {
        const auto &res = shadow.spot_[i];
        const float texel = 1.0f / static_cast<float>(res.texture.width());
        res.texture.bind(unit);
        shader.setUniformIfPresent(std::format("shadowMap.spot[{}].map", i),
                                   static_cast<int>(unit));
        shader.setUniformIfPresent(std::format("shadowMap.spot[{}].transform", i),
                                   res.params.transform);
        shader.setUniformIfPresent(std::format("shadowMap.spot[{}].bias.min_bias", i),
                                   res.params.bias.min_bias);
        shader.setUniformIfPresent(std::format("shadowMap.spot[{}].bias.slope_bias", i),
                                   res.params.bias.slope_bias);
        shader.setUniformIfPresent(std::format("shadowMap.spot[{}].texel_size", i),
                                   glm::vec2{ texel, texel });
    }
    for (size_t i = 0; i < shadow.point_.size(); ++i, ++unit) {
        const auto &res = shadow.point_[i];
        const float texel = 1.0f / static_cast<float>(res.texture.width());
        res.texture.bind(unit);
        shader.setUniformIfPresent(std::format("shadowMap.point[{}].map", i),
                                   static_cast<int>(unit));
        shader.setUniformIfPresent(std::format("shadowMap.point[{}].bias.min_bias", i),
                                   res.params.bias.min_bias);
        shader.setUniformIfPresent(std::format("shadowMap.point[{}].bias.slope_bias", i),
                                   res.params.bias.slope_bias);
        shader.setUniformIfPresent(std::format("shadowMap.point[{}].far_plane", i),
                                   res.params.far_plane);
        shader.setUniformIfPresent(std::format("shadowMap.point[{}].texel_size", i),
                                   glm::vec2{ texel, texel });
    }
}

void Renderer::applyMaterial(const Material &material, ShaderProgram &program, unsigned first_unit)
{
    unsigned bp = first_unit;

    if (material.quad_texture_) {
        if (const GLint loc = program.uniformLocation("quad_texture"); loc >= 0) {
            material.quad_texture_->bind(bp);
            program.setInt(loc, static_cast<int>(bp));
        }
        return;
    }

    if (const GLint loc = program.uniformLocation("skybox"); loc >= 0) {
        if (material.skybox_) {
            material.skybox_->bind(bp);
            program.setInt(loc, static_cast<int>(bp));
        }
        return;
    }

    program.setUniformIfPresent("material.diffuse_exist", material.diffuse_.has_value());
    if (material.diffuse_) {
        if (const GLint loc = program.uniformLocation("material.diffuse_texture"); loc >= 0) {
            material.diffuse_->bind(bp);
            program.setInt(loc, static_cast<int>(bp));
            ++bp;
        }
    }

    program.setUniformIfPresent("material.specular_exist", material.specular_.has_value());
    if (material.specular_) {
        if (const GLint loc = program.uniformLocation("material.specular_texture"); loc >= 0) {
            material.specular_->bind(bp);
            program.setInt(loc, static_cast<int>(bp));
            ++bp;
        }
    }

    program.setUniformIfPresent("material.normal_exist", material.normal_.has_value());
    if (material.normal_) {
        if (const GLint loc = program.uniformLocation("material.normal_map"); loc >= 0) {
            material.normal_->bind(bp);
            program.setInt(loc, static_cast<int>(bp));
            ++bp;
        }
    }

    program.setUniformIfPresent("material.height_exist", material.height_.has_value());
    if (material.height_) {
        if (const GLint loc = program.uniformLocation("material.height_map"); loc >= 0) {
            material.height_->bind(bp);
            program.setInt(loc, static_cast<int>(bp));
            ++bp;
        }
    }
    program.setUniformIfPresent("material.height_scale", material.height_scale_);
    program.setUniformIfPresent("material.shininess", material.shininess_);
    program.setUniformIfPresent("material.pure_color", material.pure_color_);
    program.setUniformIfPresent("material.color", material.color_);
    program.setUniformIfPresent("material.reflect_cube_exist", false);
    program.setUniformIfPresent("material.refract_cube_exist", false);
}

std::vector<std::pair<Model *, float>> Renderer::sortObjectsByDistance(
    std::vector<Model> &models)
{
    std::vector<std::pair<Model *, float>> sorted;
    sorted.reserve(models.size());
    const glm::vec3 camera_position{ camera_data_.pos };
    for (auto &model : models) {
        float distance = 0.0f;
        if (!model.objects_.empty()) {
            const auto &instances = model.objects_.front().mesh_.instanceBuffer().data();
            if (!instances.empty()) {
                distance = glm::distance(camera_position, instances.front().translation_);
            }
        }
        sorted.emplace_back(&model, distance);
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) {
        return a.second > b.second;
    });
    return sorted;
}
