#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <map>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Scene/Camera.h"
#include "Scene/Scene.h"

struct WindowState {
    Camera camera{ Camera::Type::Fly, glm::vec3{ 0.0f, 0.0f, 0.0f } };
    int width{ 0 };
    int height{ 0 };
    float fov{ 45.0f };
    float exposure{ 5.0f };
    float gamma{ 2.2f };
    bool first_mouse{ true };
    double mouse_x{ 0.0 };
    double mouse_y{ 0.0 };
    float move_speed{ 5.0f };
    float rotate_sensitivity{ 0.05f };
};

class GLFWWin {
public:
    // start_tp：场景起始时刻；delta_time：距上一帧的间隔
    using FrameAction = std::function<void(std::chrono::steady_clock::time_point start_tp,
                                           std::chrono::duration<float> delta_time)>;
    // 注册动作的句柄，用于反注册
    using ActionId = std::size_t;

    GLFWWin(int width, int height, const char *title, WindowState &state);
    ~GLFWWin();

    GLFWWin(const GLFWWin &) = delete;
    GLFWWin &operator=(const GLFWWin &) = delete;

    GLFWwindow *get() const noexcept;
    bool shouldClose() const;

    ActionId addBeginFrameAction(FrameAction action);
    void removeBeginFrameAction(ActionId id);

    ActionId addEndFrameAction(FrameAction action);
    void removeEndFrameAction(ActionId id);

    void beginFrame(Scene &scene);
    void endFrame(Scene &scene);

private:
    using ActionMap = std::map<ActionId, FrameAction>;

    ActionId addAction(ActionMap &actions, FrameAction action);
    static void runActions(const ActionMap &actions,
                           std::chrono::steady_clock::time_point start_tp,
                           std::chrono::duration<float> delta_time);
    void processInput(std::chrono::duration<float> delta_time);
    static void updateCameraProjection(WindowState &state);

    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    GLFWwindow *window_{ nullptr };
    WindowState &state_;
    // beginFrame 算出的本帧间隔，endFrame 复用（Scene 里只存时刻，无法反推）
    std::chrono::duration<float> delta_time_{ 0.0f };
    ActionMap begin_actions_;
    ActionMap end_actions_;
    ActionId next_action_id_{ 0 };

    static int ref_count_; // 已成功构造的 GLFWWin 数量
};
