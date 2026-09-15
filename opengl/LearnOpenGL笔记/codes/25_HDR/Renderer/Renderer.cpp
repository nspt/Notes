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
    initFrameBuffers();
    initShadowResources();
}

Renderer::~Renderer() {}

void Renderer::setScene(Scene scene)
{
    scene_ = std::move(scene);
    light_ubo_.setSubData(0, sizeof(LightData), &scene_.data_->lights_);
    reallocFrameBuffers();
    initShadowResources();
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
    applyShadow();
    forwardPass();
    postProcPass();
}

void Renderer::initFrameBuffers()
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
    color.setFilterMode(GL_LINEAR, GL_LINEAR);
    bright.setFilterMode(GL_LINEAR, GL_LINEAR);
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
    blur_color.setFilterMode(GL_LINEAR, GL_LINEAR);
    blur_color.setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    blur_framebuffer_.attachTexture(GL_COLOR_ATTACHMENT0, blur_color);
    const GLenum blur_bufs[] = { GL_COLOR_ATTACHMENT0 };
    blur_framebuffer_.drawBuffers(1, blur_bufs);
    if (!blur_framebuffer_.isCompleted()) {
        throw std::runtime_error{ "Bloom blur FBO is not completed" };
    }

    Texture2D composite_color{ width, height, color_fmt, GL_RGB, nullptr };
    composite_color.setFilterMode(GL_LINEAR, GL_LINEAR);
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

void Renderer::initShadowResources()
{
    shadow_maps_ = {};
    for (size_t i = 0; i < MAX_DIRECTIONAL_LIGHT; ++i) {
        auto &l{ scene_.data_->lights_.directional_[i] };
        auto &s{ shadow_maps_.directional_[i] };
        int tex_w = static_cast<int>(roundf(l.shadow_.z));
        int tex_h = static_cast<int>(roundf(l.shadow_.w));
        if (!tex_w || !tex_h) {
            continue;
        }
        s.texture_ = createShadowMapTexture(tex_w, tex_h);
        s.fb_ = createShadowMapFBO(shadow_maps_.directional_[i].texture_);
    }
    for (size_t i = 0; i < MAX_SPOT_LIGHT; ++i) {
        auto &l{ scene_.data_->lights_.spot_[i] };
        auto &s{ shadow_maps_.spot_[i] };
        int tex_w = static_cast<int>(roundf(l.shadow_.z));
        int tex_h = static_cast<int>(roundf(l.shadow_.w));
        if (!tex_w || !tex_h) {
            continue;
        }
        s.texture_ = createShadowMapTexture(tex_w, tex_h);
        s.fb_ = createShadowMapFBO(shadow_maps_.spot_[i].texture_);
    }
    for (size_t i = 0; i < MAX_POINT_LIGHT; ++i) {
        auto &l{ scene_.data_->lights_.point_[i] };
        auto &s{ shadow_maps_.point_[i] };
        int tex_w = static_cast<int>(roundf(l.shadow_.z));
        int tex_h = static_cast<int>(roundf(l.shadow_.w));
        if (!tex_w || !tex_h) {
            continue;
        }
        s.texture_ = createShadowMapTextureCube(tex_w, tex_h);
        s.fb_ = createShadowMapFBO(shadow_maps_.point_[i].texture_);
    }
}

void Renderer::shadowPass()
{
    PipelineState shadow_state;
    shadow_state.cull_face_mode_ = GL_FRONT;
    shadow_state.apply();

    for (size_t i = 0; i < MAX_DIRECTIONAL_LIGHT; ++i) {
        auto &l{ scene_.data_->lights_.directional_[i] };
        if (l.shadow_.z == 0) {
            continue;
        }
        auto &s{ shadow_maps_.directional_[i] };
        s.fb_.bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, s.texture_.width(), s.texture_.height());
        renderDirShadow(scene_.data_->models_, l.transform_);
        renderDirShadow(scene_.data_->transparent_models_, l.transform_);
    }
    for (size_t i = 0; i < MAX_SPOT_LIGHT; ++i) {
        auto &l{ scene_.data_->lights_.spot_[i] };
        if (l.shadow_.z == 0) {
            continue;
        }
        auto &s{ shadow_maps_.spot_[i] };
        s.fb_.bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, s.texture_.width(), s.texture_.height());
        renderDirShadow(scene_.data_->models_, l.transform_);
        renderDirShadow(scene_.data_->transparent_models_, l.transform_);
    }
    for (size_t i = 0; i < MAX_POINT_LIGHT; ++i) {
        auto &l{ scene_.data_->lights_.point_[i] };
        if (l.shadow_.z == 0) {
            continue;
        }
        auto &s{ shadow_maps_.point_[i] };
        s.fb_.bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, s.texture_.width(), s.texture_.height());
        const glm::vec3 light_pos{ l.position_ };
        const float far_plane = l.attenuation_.w;
        const auto transforms = calcPointLightSpaceTransforms(light_pos, 1.0f, far_plane);
        renderOmniShadow(scene_.data_->models_, transforms.data(), light_pos, far_plane);
        renderOmniShadow(scene_.data_->transparent_models_, transforms.data(), light_pos, far_plane);
    }
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

    bool horizontal = true;
    for (int i = 0; i < bloom_blur_iterations_ * 2; ++i) {
        // horizontal → blur FBO（读 resolve att1）；vertical → resolve att1（读 blur FBO）
        if (horizontal) {
            blur_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT0);
            bright_color.bind(BLOOM_IMAGE_UNIT);
        } else {
            resolve_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT1);
            blur_color.bind(BLOOM_IMAGE_UNIT);
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

    if (bloom_enabled_) {
        applyBloomThreshold();
    }

    auto draw_model = [&](Model &model) {
        for (auto &object : model.objects_) {
            auto &program = object.render_shader_;
            if (program.id() == 0) {
                continue;
            }
            object.pipeline_state_.apply();
            program.use();
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

    if (scene_.data_->skybox_) {
        // skybox 只写 scene 颜色，不进 bloom 高光目标
        hdr_framebuffer_.drawBuffer(GL_COLOR_ATTACHMENT0);
        draw_model(*scene_.data_->skybox_);
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
        blurBrightPass(std::get<2>(viewport_), std::get<3>(viewport_));
        compositePass();
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

    scene_color.bind(COMPOSITE_SCENE_UNIT);
    bloom_color.bind(COMPOSITE_BLOOM_UNIT);
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

    composite_color.bind(QUAD_TEXTURE_UNIT);
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


void Renderer::renderDirShadow(const std::vector<Model> &models, const glm::mat4 &light_space)
{
    for (auto &model : models) {
        renderDirShadow(model, light_space);
    }
}

void Renderer::renderDirShadow(const Model &model, const glm::mat4 &light_space)
{
    for (auto &object : model.objects_) {
        auto &shader = object.directional_shadow_shader_;
        if (shader.id() == 0)
            continue;
        shader.setMat4("light_space_transform", light_space);
        object.mesh_.draw(static_cast<GLsizei>(object.mesh_.instanceCount()));
    }
}
void Renderer::renderOmniShadow(const std::vector<Model> &models, const glm::mat4 *light_spaces, const glm::vec3 &light_pos, float far_plane)
{
    for (auto &model : models) {
        renderOmniShadow(model, light_spaces, light_pos, far_plane);
    }
}

void Renderer::renderOmniShadow(const Model &model, const glm::mat4 *light_spaces, const glm::vec3 &light_pos, float far_plane)
{
    for (auto &object : model.objects_) {
        auto &shader = object.omni_shadow_shader_;
        if (shader.id() == 0)
            continue;
        shader.setMat4Arr("light_space_transform", light_spaces, 6);
        shader.setVec3("light_pos", light_pos);
        shader.setFloat("far_plane", far_plane);
        object.mesh_.draw(static_cast<GLsizei>(object.mesh_.instanceCount()));
    }
}

void Renderer::bindUniformBuffers() const
{
    light_ubo_.bindBase(0);
    cam_data_ubo_.bindBase(1);
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

void Renderer::applyShadow()
{
    for (GLuint i = 0; i < MAX_DIRECTIONAL_LIGHT; ++i) {
        shadow_maps_.directional_[i].texture_.bind(DIR_LIGHT_START_UNIT + i);
    }
    for (GLuint i = 0; i < MAX_SPOT_LIGHT; ++i) {
        shadow_maps_.spot_[i].texture_.bind(SPOT_LIGHT_START_UNIT + i);
    }
    for (GLuint i = 0; i < MAX_POINT_LIGHT; ++i) {
        shadow_maps_.point_[i].texture_.bind(POINT_LIGHT_START_UNIT + i);
    }
}

void Renderer::applyMaterial(const Material &material, ShaderProgram &program)
{
    if (material.quad_texture_) {
        if (const GLint loc = program.uniformLocation("quad_texture"); loc >= 0) {
            material.quad_texture_->bind(QUAD_TEXTURE_UNIT);
        }
        return;
    }

    if (const GLint loc = program.uniformLocation("skybox"); loc >= 0) {
        if (material.skybox_) {
            material.skybox_->bind(SKYBOX_UNIT);
        }
        return;
    }

    program.setUniformIfPresent("material.diffuse_exist", material.diffuse_.has_value());
    if (material.diffuse_) {
        material.diffuse_->bind(DIFFUSE_UNIT);
    }

    program.setUniformIfPresent("material.specular_exist", material.specular_.has_value());
    if (material.specular_) {
        material.specular_->bind(SPECULAR_UNIT);
    }

    program.setUniformIfPresent("material.normal_exist", material.normal_.has_value());
    if (material.normal_) {
        material.normal_->bind(NORMAL_UNIT);
    }

    program.setUniformIfPresent("material.height_exist", material.height_.has_value());
    if (material.height_) {
        material.height_->bind(HEIGHT_UNIT);
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
