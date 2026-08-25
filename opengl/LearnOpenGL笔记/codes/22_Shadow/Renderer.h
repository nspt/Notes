#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "Camera.h"
#include "FrameBuffer.h"
#include "Light.h"
#include "Model.h"
#include "RenderObject.h"
#include "ShaderProgram.h"

struct WinData {
    Camera camera{ Camera::Type::Fly, glm::vec3{ 0.0f, 1.0f, 0.0f } };
    int win_width{ 0 };
    int win_height{ 0 };
    LightData lights;
    UniformBuffer lights_UBO{ sizeof(LightData) };
    CameraData cam_data;
    UniformBuffer cam_data_UBO{ sizeof(CameraData) };
    std::chrono::time_point<std::chrono::steady_clock> last_time{
        std::chrono::steady_clock::now()
    };
    std::chrono::duration<float> delta_time{ 0 };
    float fov{ 45.0f };
    bool first_mouse{ true };
    double mouse_x{ 0.0 };
    double mouse_y{ 0.0 };
    float move_speed{ 20.0f };
    float rotate_sensitivity{ 0.05f };
    float zoom_sensitivity{ 0.1f };
};

struct ShadowResources {
    static constexpr int map_size = 2048;
    static constexpr float point_near = 0.1f;
    static constexpr float point_far = 100.0f;

    ShadowData data;
    std::vector<FrameBuffer> directional_fbos;
    std::vector<FrameBuffer> spot_fbos;
    std::vector<FrameBuffer> point_fbos;
    std::vector<std::array<glm::mat4, 6>> point_light_spaces;
};

struct MirrorResources {
    static constexpr GLsizei samples = 4;
    Texture2DMS color_ms;
    RenderBufferMS depth_stencil_rbo;
    FrameBuffer fbo;
    RenderObject quad;
};

class Renderer {
public:
    explicit Renderer(int width = 1920, int height = 1080, const char *title = "Shadow");
    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    void loadShaders(const std::string &resourceDir);
    void initShadowResources();
    void initMirror();

    // 一帧：shadow → mirror → 主场景 → 镜子四边形
    void renderFrame();
    void beginFrameTiming();
    void endFrame(); // delta_time + swap + poll
    void processInput();

    bool shouldClose() const;
    GLFWwindow *window() const noexcept { return window_; }

    WinData &winData() noexcept { return *win_data_; }
    const WinData &winData() const noexcept { return *win_data_; }
    LightData &lights() noexcept { return win_data_->lights; }
    const LightData &lights() const noexcept { return win_data_->lights; }
    Camera &camera() noexcept { return win_data_->camera; }

    Camera *currentRenderCamera() const noexcept { return current_render_camera_; }

    ShaderProgram &shader(const std::string &name);
    const ShaderProgram &shader(const std::string &name) const;

    std::vector<Model> &objects() noexcept { return objects_; }
    std::vector<Model> &outlineObjects() noexcept { return outline_objects_; }
    std::vector<Model> &transparentObjects() noexcept { return transparent_objects_; }

    void clearScene();

private:
    void initGLFW(int major, int minor);
    void createWindow(int width, int height, const char *title);
    void initGL();
    void initWinData();
    void initLightData();

    void updateShadowMaps();
    void renderDirectionalShadowCasters(const glm::mat4 &light_space);
    void renderOmniShadowCasters(const glm::mat4 *light_spaces,
                                 const glm::vec3 &light_pos,
                                 float far_plane);
    void renderScene(Camera &cam);
    void resizeMirrorIfNeeded();
    void renderMirror();

    static std::vector<std::pair<Model *, float>> sortObjectsByDistance(
        Camera &camera, const std::vector<Model> &objs);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
    static Mesh createQuadMesh(InstanceBuffer ibo = {});

    GLFWwindow *window_{ nullptr };
    std::unique_ptr<WinData> win_data_;

    std::unordered_map<std::string, ShaderProgram> shaders_;
    std::vector<Model> objects_;
    std::vector<Model> outline_objects_;
    std::vector<Model> transparent_objects_;
    Camera *current_render_camera_{ nullptr };

    ShadowResources shadow_;
    std::optional<MirrorResources> mirror_;
};
