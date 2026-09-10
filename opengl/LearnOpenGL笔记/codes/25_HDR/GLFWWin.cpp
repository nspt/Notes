#include "GLFWWin.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

int GLFWWin::ref_count_ = 0;

GLFWWin::GLFWWin(int width, int height, const char *title, WindowState &state)
    : state_{ state }
{
    if (ref_count_ == 0 && !glfwInit()) {
        throw std::runtime_error{ "Failed to init GLFW" };
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        if (ref_count_ == 0) {
            glfwTerminate();
        }
        throw std::runtime_error{ "Failed to create GLFW window" };
    }
    glfwMakeContextCurrent(window_);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        if (ref_count_ == 0) {
            glfwTerminate();
        }
        throw std::runtime_error{ "Failed to init GLAD" };
    }

    state_.width = width;
    state_.height = height;
    glfwSetWindowUserPointer(window_, &state_);
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
    glfwSetScrollCallback(window_, scrollCallback);

    updateCameraProjection(state_);
    ++ref_count_;
}

GLFWWin::~GLFWWin()
{
    if (window_) {
        glfwDestroyWindow(window_);
    }
    if (--ref_count_ == 0) {
        glfwTerminate();
    }
}

GLFWwindow *GLFWWin::get() const noexcept
{
    return window_;
}

bool GLFWWin::shouldClose() const
{
    return glfwWindowShouldClose(window_);
}

GLFWWin::ActionId GLFWWin::addBeginFrameAction(FrameAction action)
{
    return addAction(begin_actions_, std::move(action));
}

void GLFWWin::removeBeginFrameAction(ActionId id)
{
    begin_actions_.erase(id);
}

GLFWWin::ActionId GLFWWin::addEndFrameAction(FrameAction action)
{
    return addAction(end_actions_, std::move(action));
}

void GLFWWin::removeEndFrameAction(ActionId id)
{
    end_actions_.erase(id);
}

void GLFWWin::beginFrame(Scene &scene)
{
    auto &data = *scene.data_;
    const auto now = std::chrono::steady_clock::now();
    if (!data.start_tp_) {
        data.start_tp_ = now;
    }
    delta_time_ = data.last_update_tp_
        ? std::chrono::duration<float>(now - *data.last_update_tp_)
        : std::chrono::duration<float>{ 0.0f };
    data.last_update_tp_ = now;
    const auto start_tp = *data.start_tp_;

    processInput(delta_time_);

    runActions(begin_actions_, start_tp, delta_time_);

    auto run_update_actions = [&](std::vector<Model> &models) {
        for (auto &model : models) {
            if (model.update_action_) {
                (*model.update_action_)(model, start_tp, delta_time_);
            }
        }
    };
    run_update_actions(data.models_);
    run_update_actions(data.transparent_models_);
}

void GLFWWin::endFrame(Scene &scene)
{
    if (const auto &start_tp = scene.data_->start_tp_) {
        runActions(end_actions_, *start_tp, delta_time_);
    }
    glfwSwapBuffers(window_);
    glfwPollEvents();
}

GLFWWin::ActionId GLFWWin::addAction(ActionMap &actions, FrameAction action)
{
    const ActionId id = next_action_id_++;
    actions.emplace(id, std::move(action));
    return id;
}

void GLFWWin::runActions(const ActionMap &actions,
                         std::chrono::steady_clock::time_point start_tp,
                         std::chrono::duration<float> delta_time)
{
    for (const auto &[id, action] : actions) {
        (void)id;
        if (action) {
            action(start_tp, delta_time);
        }
    }
}

void GLFWWin::processInput(std::chrono::duration<float> delta_time)
{
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, true);
        return;
    }

    auto &camera = state_.camera;
    float distance = state_.move_speed * delta_time.count();
    auto front = camera.front();
    auto right = camera.right();
    if (camera.type() == Camera::Type::FPS) {
        front.y = 0;
        front = glm::normalize(front);
        right.y = 0;
        right = glm::normalize(right);
    }
    glm::vec3 world_up{ 0.0f, 1.0f, 0.0f };
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
        camera.move(front * distance);
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
        camera.move(-front * distance);
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) {
        camera.move(-right * distance);
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
        camera.move(right * distance);
    }
    if (camera.type() == Camera::Type::Fly && glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.move(world_up * distance);
    }

    const float exposure_step = 1.0f * delta_time.count();
    bool exposure_changed = false;
    if (glfwGetKey(window_, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        const float prev = state_.exposure;
        state_.exposure = std::max(0.0f, state_.exposure - exposure_step);
        exposure_changed = state_.exposure != prev;
    }
    if (glfwGetKey(window_, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        state_.exposure += exposure_step;
        exposure_changed = true;
    }
    if (exposure_changed) {
        std::cout << "exposure: " << state_.exposure << '\n';
    }

    const float gamma_step = 1.0f * delta_time.count();
    bool gamma_changed = false;
    if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
        const float prev = state_.gamma;
        state_.gamma = std::max(0.1f, state_.gamma - gamma_step);
        gamma_changed = state_.gamma != prev;
    }
    if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
        state_.gamma += gamma_step;
        gamma_changed = true;
    }
    if (gamma_changed) {
        std::cout << "gamma: " << state_.gamma << '\n';
    }

    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    if (state_.first_mouse) {
        state_.first_mouse = false;
        state_.mouse_x = x;
        state_.mouse_y = y;
    } else {
        if (camera.type() == Camera::Type::Fly
            && glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            camera.move(glm::vec3{ 0.0f, -1.0f, 0.0f } * distance);
        }
        auto delta_x = x - state_.mouse_x;
        auto delta_y = y - state_.mouse_y;
        state_.mouse_x = x;
        state_.mouse_y = y;
        camera.yaw(-state_.rotate_sensitivity * delta_x);
        camera.pitch(-state_.rotate_sensitivity * delta_y);
    }
}

void GLFWWin::updateCameraProjection(WindowState &state)
{
    state.camera.setProjection(glm::perspective(
        glm::radians(state.fov),
        static_cast<float>(state.width) / static_cast<float>(std::max(state.height, 1)),
        0.1f,
        100000.0f
    ));
}

void GLFWWin::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    auto *win = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
    win->width = width;
    win->height = height;
    glViewport(0, 0, width, height);
    updateCameraProjection(*win);
}

void GLFWWin::scrollCallback(GLFWwindow *window, double, double yoffset)
{
    auto *win = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
    win->fov = glm::clamp(win->fov - static_cast<float>(yoffset), 1.0f, 45.0f);
    updateCameraProjection(*win);
}
