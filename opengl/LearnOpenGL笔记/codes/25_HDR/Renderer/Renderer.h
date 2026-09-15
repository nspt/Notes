#pragma once

#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "../Scene/Scene.h"
#include "../Scene/Camera.h"
#include "../FrameBuffer/FrameBuffer.h"
#include "../Model/RenderObject.h"
#include "ShaderProgram.h"
#include "../Buffers/UniformBuffer.h"

class Renderer {
public:
    // lit / explode
    static inline constexpr GLuint DIFFUSE_UNIT = 0;
    static inline constexpr GLuint SPECULAR_UNIT = 1;
    static inline constexpr GLuint NORMAL_UNIT = 2;
    static inline constexpr GLuint HEIGHT_UNIT = 3;
    static inline constexpr GLuint REFLECT_CUBE_UNIT = 4;
    static inline constexpr GLuint REFRACT_CUBE_UNIT = 5;
    static inline constexpr GLuint DIR_LIGHT_START_UNIT = 6;
    static inline constexpr GLuint SPOT_LIGHT_START_UNIT = DIR_LIGHT_START_UNIT + MAX_DIRECTIONAL_LIGHT;
    static inline constexpr GLuint POINT_LIGHT_START_UNIT = SPOT_LIGHT_START_UNIT + MAX_SPOT_LIGHT;
    // 其它 pass 可复用低编号 unit（不同类型 shader）
    static inline constexpr GLuint SKYBOX_UNIT = 0;
    static inline constexpr GLuint BLOOM_IMAGE_UNIT = 0;
    static inline constexpr GLuint COMPOSITE_SCENE_UNIT = 0;
    static inline constexpr GLuint COMPOSITE_BLOOM_UNIT = 1;
    static inline constexpr GLuint QUAD_TEXTURE_UNIT = 0;
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

    void applyShadow();
    void applyMaterial(const Material &material, ShaderProgram &shader);

private:
    void initFrameBuffers();
    void reallocFrameBuffers();
    void bindFinalFrameBuffer() const;
    GLenum colorInternalFormat() const noexcept;

    void initShadowResources();
    void bindUniformBuffers() const;
    void applyBloomThreshold();

    void shadowPass();
    void forwardPass();
    void compositePass();
    void postProcPass();

    void renderDirShadow(const std::vector<Model> &models, const glm::mat4 &light_space);
    void renderDirShadow(const Model &model, const glm::mat4 &light_space);
    void renderOmniShadow(const std::vector<Model> &models, const glm::mat4 *light_spaces, const glm::vec3 &light_pos, float far_plane);
    void renderOmniShadow(const Model &model, const glm::mat4 *light_spaces, const glm::vec3 &light_pos, float far_plane);

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
    ShadowMaps shadow_maps_;

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

    float bloom_threshold_{ 1.0f };
    bool hdr_enabled_{ true };
    bool bloom_enabled_{ true };

    static ShaderMap shaders_;
    static constexpr GLsizei hdr_samples_ = 4;
    static constexpr int bloom_blur_iterations_ = 5;
};
