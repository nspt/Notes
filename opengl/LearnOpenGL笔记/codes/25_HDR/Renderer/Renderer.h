#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "../Scene/Scene.h"
#include "../Scene/Shadow.h"
#include "../Scene/Camera.h"
#include "../FrameBuffer/FrameBuffer.h"
#include "../Model/RenderObject.h"
#include "ShaderProgram.h"
#include "PipelineState.h"
#include "../Buffers/UniformBuffer.h"

class Renderer {
public:
    using ShaderMap = std::map<std::string, ShaderProgram>;
    struct PostProcParams {
        bool enable_kernel{ false };
        float kernel[9]{
            0, 0, 0,
            0, 1, 0,
            0, 0, 0
        };
        float exposure{ 1.0 };
        float gamma{ 2.2 };
    };

    Renderer();
    ~Renderer();

    void setScene(Scene scene);
    Scene scene() const;

    void setCameraData(const CameraData &cam);
    CameraData cameraData() const;

    void setShaders(ShaderMap shaders);
    ShaderMap allShaders() const;
    ShaderProgram shader(const std::string &name) const;

    void setFrameBuffer(FrameBuffer fb);
    FrameBuffer frameBuffer() const;
    void resetFrameBuffer();

    void setViewport(int x, int y, int width, int height);
    std::tuple<int, int, int, int> viewport() const;

    void setPostProcParams(PostProcParams params);
    PostProcParams postProcParams() const;

    void setPostProcShader(ShaderProgram shader);
    void setBlurShader(ShaderProgram shader);
    void setCompositeShader(ShaderProgram shader);

    void setBloomThreshold(float threshold);
    float bloomThreshold() const;

    void setHdrEnabled(bool enabled);
    bool hdrEnabled() const;
    void setBloomEnabled(bool enabled);
    bool bloomEnabled() const;

    void render();

    static void applyShadow(const ShadowResources &shadow, ShaderProgram &shader, unsigned first_unit = 16);
    static void applyMaterial(const Material &material, ShaderProgram &program, unsigned first_unit = 0);

private:
    void createFrameBuffers();
    void reallocFrameBuffers();
    void bindFinalFrameBuffer() const;
    GLenum colorInternalFormat() const noexcept;

    void createShadowResources(int shadow_map_size = 1024);
    void bindUniformBuffers() const;
    void applySceneShadows();
    void applyBloomThreshold();

    void shadowPass();
    void forwardPass();
    void compositePass();
    void postProcPass();

    void renderDirShadow(const glm::mat4 &light_space);
    void renderOmniShadow(const glm::mat4 *light_spaces, const glm::vec3 &light_pos, float far_plane);

    void blurBrightPass(int width, int height);
    void blitColorAttachment(FrameBuffer &src, FrameBuffer *dst,
                             GLenum src_attachment, GLenum dst_attachment,
                             int src_w, int src_h,
                             int dst_x, int dst_y, int dst_w, int dst_h);
    void drawFullscreenQuad(ShaderProgram &program);

    std::vector<std::pair<Model *, float>> sortObjectsByDistance(std::vector<Model> &models);

    Scene scene_;
    CameraData camera_data_{};
    UniformBuffer cam_data_ubo_{ sizeof(CameraData) };
    UniformBuffer light_ubo_{ sizeof(LightData) };
    ShadowResources shadow_resources_;

    std::tuple<int, int, int, int> viewport_{ 0, 0, 1, 1 };

    FrameBuffer hdr_framebuffer_;
    FrameBuffer resolve_framebuffer_;
    FrameBuffer blur_framebuffer_;
    FrameBuffer composite_framebuffer_;

    PostProcParams post_proc_params_{};
    RenderObject post_proc_quad_;
    std::optional<ShaderProgram> post_proc_shader_;
    std::optional<ShaderProgram> blur_shader_;
    std::optional<ShaderProgram> composite_shader_;
    std::optional<FrameBuffer> final_framebuffer_;

    static ShaderMap shaders_;
    static constexpr GLsizei hdr_samples_ = 4;
    static constexpr int bloom_blur_iterations_ = 5;
    float bloom_threshold_{ 1.0f };
    bool hdr_enabled_{ true };
    bool bloom_enabled_{ true };
};
