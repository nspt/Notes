#include "Renderer.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

#include "Buffers.h"
#include "Mesh.h"

Renderer::Renderer(int width, int height, const char *title)
{
    initGLFW(3, 3);
    createWindow(width, height, title);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error{ "Failed to init GLAD" };
    }
    initGL();
    win_data_ = std::make_unique<WinData>();
    win_data_->win_width = width;
    win_data_->win_height = height;
    initWinData();
    glfwSetWindowUserPointer(window_, win_data_.get());
}

Renderer::~Renderer()
{
    clearScene();
    shaders_.clear();
    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Renderer::initGLFW(int major, int minor)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
}

void Renderer::createWindow(int width, int height, const char *title)
{
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        throw std::runtime_error{ "Failed to create GLFW window" };
    }
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);
    glfwSetScrollCallback(window_, scrollCallback);
    glfwMakeContextCurrent(window_);
}

void Renderer::initGL()
{
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
}

void Renderer::initLightData()
{
    auto &data = win_data_->lights;

    data.counts.x = 1;
    data.directional[0].direction_ = glm::normalize(glm::vec4{ -1, -1, -1, 0 });
    data.directional[0].ambient_ = glm::vec4{ 0.05 };
    data.directional[0].diffuse_ = glm::vec4{ 0.3 };
    data.directional[0].specular_ = glm::vec4{ 0.5 };
    data.directional[0].light_space_transform_ = calcDirectionalLightSpaceTransform(
        glm::vec3(data.directional[0].direction_), 20.0f, 1.0f, 100.0f
    );

    data.counts.y = 1;
    data.point[0].pos_ = glm::vec4{ 10.0f, 0.0f, 0.0f, 1.0f };
    data.point[0].ambient_ = glm::vec4{ 0.05f };
    data.point[0].diffuse_ = glm::vec4{ 1.0f };
    data.point[0].specular_ = glm::vec4{ 1.0f };
    data.point[0].attenuation = glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f };

    data.counts.z = 1;
    data.spot[0].pos_ = glm::vec4{ 0, 10, 10, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[0].pos_));
        data.spot[0].direction_inner_ = glm::vec4{ dir, glm::cos(glm::radians(22.5f)) };
    }
    data.spot[0].ambient_ = glm::vec4{ 0.0 };
    data.spot[0].diffuse_ = glm::vec4{ 0.3 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    data.spot[0].attenuation_outter_ = glm::vec4{
        1.0, 0.0009, 0.00032, glm::cos(glm::radians(37.5f))
    };
    data.spot[0].light_space_transform_ = calcSpotLightSpaceTransform(
        glm::vec3(data.spot[0].pos_),
        glm::vec3(data.spot[0].direction_inner_),
        data.spot[0].attenuation_outter_.w,
        0.1f, 100.0f
    );

    data.spot[1].pos_ = glm::vec4{ 10, 10, 0, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[1].pos_));
        data.spot[1].direction_inner_ = glm::vec4{ dir, glm::cos(glm::radians(12.5f)) };
    }
    data.spot[1].ambient_ = glm::vec4{ 0.1 };
    data.spot[1].diffuse_ = glm::vec4{ 1.0 };
    data.spot[1].specular_ = glm::vec4{ 1.0 };
    data.spot[1].attenuation_outter_ = glm::vec4{
        1.0, 0.09, 0.032, glm::cos(glm::radians(17.5f))
    };
    data.spot[1].light_space_transform_ = calcSpotLightSpaceTransform(
        glm::vec3(data.spot[1].pos_),
        glm::vec3(data.spot[1].direction_inner_),
        data.spot[1].attenuation_outter_.w,
        0.1f, 50.0f
    );
}

void Renderer::initWinData()
{
    win_data_->fov = 45.0f;
    win_data_->cam_data.pos = glm::vec4{ win_data_->camera.pos(), 1.0f };
    win_data_->cam_data.view = win_data_->camera.viewMatrix();
    win_data_->cam_data.projection = glm::perspective(
        glm::radians(win_data_->fov),
        static_cast<float>(win_data_->win_width) / static_cast<float>(win_data_->win_height),
        0.1f,
        100000.0f
    );
    initLightData();

    win_data_->lights_UBO.bindBase(0);
    win_data_->lights_UBO.setSubData(0, sizeof(LightData), &win_data_->lights);
    win_data_->cam_data_UBO.bindBase(1);
    win_data_->cam_data_UBO.setSubData(0, sizeof(CameraData), &win_data_->cam_data);
}

void Renderer::loadShaders(const std::string &resourceDir)
{
    auto add = [&](const std::string &name,
                   const std::filesystem::path &vert,
                   const std::filesystem::path &frag,
                   const std::filesystem::path &geom = {}) {
        shaders_.emplace(name, ShaderProgram{ vert, frag, geom });
    };

    add("general",
        resourceDir + "shaders/23_normal_map/general.vert",
        resourceDir + "shaders/23_normal_map/general.frag");
    shaders_.at("general").setUniformBlockBinding("LightData", 0);
    shaders_.at("general").setUniformBlockBinding("CamData", 1);

    add("shadow",
        resourceDir + "shaders/23_normal_map/shadow.vert",
        resourceDir + "shaders/23_normal_map/shadow.frag");

    add("cube_shadow",
        resourceDir + "shaders/23_normal_map/cube_shadow.vert",
        resourceDir + "shaders/23_normal_map/cube_shadow.frag",
        resourceDir + "shaders/23_normal_map/cube_shadow.geom");

    add("visual_normal",
        resourceDir + "shaders/23_normal_map/visual_normal.vert",
        resourceDir + "shaders/23_normal_map/visual_normal.frag",
        resourceDir + "shaders/23_normal_map/visual_normal.geom");
    shaders_.at("visual_normal").setUniformBlockBinding("CamData", 1);
    shaders_.at("visual_normal").setVec3("normal_color", glm::vec3{ 0, 1, 0 });

    add("explode",
        resourceDir + "shaders/23_normal_map/explode.vert",
        resourceDir + "shaders/23_normal_map/explode.frag",
        resourceDir + "shaders/23_normal_map/explode.geom");
    shaders_.at("explode").setUniformBlockBinding("LightData", 0);
    shaders_.at("explode").setUniformBlockBinding("CamData", 1);

    add("explode_shadow",
        resourceDir + "shaders/23_normal_map/shadow_explode.vert",
        resourceDir + "shaders/23_normal_map/shadow_explode.frag",
        resourceDir + "shaders/23_normal_map/shadow_explode.geom");

    add("skybox",
        resourceDir + "shaders/23_normal_map/skybox.vert",
        resourceDir + "shaders/23_normal_map/skybox.frag");
    shaders_.at("skybox").setUniformBlockBinding("CamData", 1);
    shaders_.at("skybox").setInt("skybox", 0);

    add("kernel",
        resourceDir + "shaders/23_normal_map/kernel.vert",
        resourceDir + "shaders/23_normal_map/kernel.frag");

    applyShadowLightingUniforms(shaders_.at("general"), win_data_->lights);
    applyShadowLightingUniforms(shaders_.at("explode"), win_data_->lights);
}

void Renderer::initShadowResources()
{
    return;
    shadow_ = ShadowResources{};
    const auto &lights = win_data_->lights;

    for (int i = 0; i < lights.counts.x; ++i) {
        shadow_.data.directional[i] = {
            createShadowMapTexture(ShadowResources::map_size), 0.00001f
        };
        shadow_.directional_fbos.push_back(createShadowMapFBO(shadow_.data.directional[i].first));
    }
    shadow_.data.counts.x = lights.counts.x;

    for (int i = 0; i < lights.counts.y; ++i) {
        shadow_.data.point[i] = {
            createShadowMapTextureCube(ShadowResources::map_size), 0.15f
        };
        shadow_.data.point_far[i] = ShadowResources::point_far;
        shadow_.point_fbos.push_back(createShadowMapFBO(shadow_.data.point[i].first));
        shadow_.point_light_spaces.push_back(calcPointLightSpaceTransforms(
            glm::vec3(lights.point[i].pos_),
            ShadowResources::point_near,
            ShadowResources::point_far
        ));
    }
    shadow_.data.counts.y = lights.counts.y;

    for (int i = 0; i < lights.counts.z; ++i) {
        shadow_.data.spot[i] = {
            createShadowMapTexture(ShadowResources::map_size), 0.00001f
        };
        shadow_.spot_fbos.push_back(createShadowMapFBO(shadow_.data.spot[i].first));
    }
    shadow_.data.counts.z = lights.counts.z;
}

Mesh Renderer::createQuadMesh(InstanceBuffer ibo)
{
    static std::vector<Vertex> vertices = {
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    static std::vector<std::uint32_t> indices = {
        0, 1, 2,
        0, 2, 3,
        0, 2, 1,
        0, 3, 2
    };
    static VertexBuffer vbo{ vertices };
    static IndexBuffer ebo{ indices };
    return Mesh{ vbo, ebo, std::move(ibo) };
}

void Renderer::initMirror()
{
    const int width = win_data_->win_width;
    const int height = win_data_->win_height / 2;
    MirrorResources mirror{
        Texture2DMS{ width, height, GL_RGB, MirrorResources::samples },
        RenderBufferMS{ GL_DEPTH24_STENCIL8, width, height, MirrorResources::samples },
        FrameBuffer{},
        RenderObject{}
    };
    mirror.fbo.attachTexture(GL_COLOR_ATTACHMENT0, mirror.color_ms);
    mirror.fbo.attachRBO(GL_DEPTH_STENCIL_ATTACHMENT, mirror.depth_stencil_rbo);
    if (!mirror.fbo.isCompleted()) {
        throw std::runtime_error{ "Mirror FBO is not completed" };
    }

    mirror.quad.render_shader_ = shaders_.at("kernel");
    float identity_kernel[9] {
        0, 0, 0,
        0, 1, 0,
        0, 0, 0
    };
    mirror.quad.render_shader_.setInt("tex.samples", MirrorResources::samples);
    mirror.quad.render_shader_.setInt("tex.tex_MS", 0);
    mirror.quad.render_shader_.setFLoatArr("kernel", identity_kernel, 9);
    mirror.quad.render_state_.depth_test_ = false;
    mirror.quad.mesh_ = createQuadMesh(
        InstanceBuffer::InstanceData{
            .translation_ = glm::vec3{ 0, 0.85, 0 },
            .scale_ = glm::vec3{ 0.3, 0.15, 1.0 }
        }
    );
    mirror_ = std::move(mirror);
}

ShaderProgram &Renderer::shader(const std::string &name)
{
    return shaders_.at(name);
}

const ShaderProgram &Renderer::shader(const std::string &name) const
{
    return shaders_.at(name);
}

void Renderer::clearScene()
{
    objects_.clear();
    outline_objects_.clear();
    transparent_objects_.clear();
}

bool Renderer::shouldClose() const
{
    return glfwWindowShouldClose(window_);
}

void Renderer::beginFrameTiming()
{
    win_data_->last_time = std::chrono::steady_clock::now();
}

void Renderer::endFrame()
{
    auto now = std::chrono::steady_clock::now();
    win_data_->delta_time = now - win_data_->last_time;
    win_data_->last_time = now;
    glfwSwapBuffers(window_);
    glfwPollEvents();
}

void Renderer::processInput()
{
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, true);
        return;
    }

    auto &camera = win_data_->camera;
    float distance = win_data_->move_speed * win_data_->delta_time.count();
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

    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window_, &x, &y);
    if (win_data_->first_mouse) {
        win_data_->first_mouse = false;
        win_data_->mouse_x = x;
        win_data_->mouse_y = y;
    } else {
        if (camera.type() == Camera::Type::Fly
            && glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            camera.move(glm::vec3{ 0.0f, -1.0f, 0.0f } * distance);
        }
        auto delta_x = x - win_data_->mouse_x;
        auto delta_y = y - win_data_->mouse_y;
        win_data_->mouse_x = x;
        win_data_->mouse_y = y;
        camera.yaw(-win_data_->rotate_sensitivity * delta_x);
        camera.pitch(-win_data_->rotate_sensitivity * delta_y);
    }

    win_data_->cam_data.pos = glm::vec4{ camera.pos(), 1.0f };
    win_data_->cam_data.view = camera.viewMatrix();
    win_data_->cam_data_UBO.setSubData(0, sizeof(CameraData), &win_data_->cam_data);
}

void Renderer::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    auto *data = static_cast<WinData *>(glfwGetWindowUserPointer(window));
    data->win_width = width;
    data->win_height = height;
    glViewport(0, 0, width, height);
    data->cam_data.projection = glm::perspective(
        glm::radians(data->fov),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        100000.0f
    );
    data->cam_data_UBO.setSubData(0, sizeof(CameraData), &data->cam_data);
}

void Renderer::scrollCallback(GLFWwindow *window, double, double yoffset)
{
    auto *data = static_cast<WinData *>(glfwGetWindowUserPointer(window));
    data->fov -= static_cast<float>(yoffset);
    if (data->fov < 1.0f) {
        data->fov = 1.0f;
    }
    if (data->fov > 45.0f) {
        data->fov = 45.0f;
    }
    data->cam_data.projection = glm::perspective(
        glm::radians(data->fov),
        static_cast<float>(data->win_width) / static_cast<float>(data->win_height),
        0.1f,
        100000.0f
    );
    data->cam_data_UBO.setSubData(0, sizeof(CameraData), &data->cam_data);
}

std::vector<std::pair<Model *, float>> Renderer::sortObjectsByDistance(
    Camera &camera, const std::vector<Model> &objs)
{
    auto cam_pos = camera.pos();
    std::vector<std::pair<Model *, float>> sorted;
    sorted.reserve(objs.size());
    for (auto &obj : objs) {
        sorted.push_back({
            const_cast<Model *>(&obj),
            glm::distance(
                cam_pos,
                obj.objects_[0].mesh_.instanceBuffer().data().front().translation_
            )
        });
    }
    std::sort(sorted.begin(), sorted.end(), [](auto &a, auto &b) {
        return a.second > b.second;
    });
    return sorted;
}

void Renderer::renderDirectionalShadowCasters(const glm::mat4 &light_space)
{
    auto draw_model_shadow = [&](Model &m) {
        for (auto &obj : m.objects_) {
            if (obj.directional_shadow_shader_.id() == 0) {
                continue;
            }
            obj.directional_shadow_shader_.setMat4("light_space_transform", light_space);
            obj.renderDirectionalShadow();
        }
    };
    for (auto &m : objects_) {
        draw_model_shadow(m);
    }
    for (auto &m : outline_objects_) {
        draw_model_shadow(m);
    }
}

void Renderer::renderOmniShadowCasters(const glm::mat4 *light_spaces,
                                       const glm::vec3 &light_pos,
                                       float far_plane)
{
    auto draw_model_shadow = [&](Model &m) {
        for (auto &obj : m.objects_) {
            if (obj.omni_shadow_shader_.id() == 0) {
                continue;
            }
            auto &sh = obj.omni_shadow_shader_;
            sh.setMat4Arr("light_space_transform", light_spaces, 6);
            sh.setVec3("light_pos", light_pos);
            sh.setFloat("far_plane", far_plane);
            obj.renderOmniShadow();
        }
    };
    for (auto &m : objects_) {
        draw_model_shadow(m);
    }
    for (auto &m : outline_objects_) {
        draw_model_shadow(m);
    }
}

void Renderer::updateShadowMaps()
{
    const float explode_magnitude = std::abs(static_cast<float>(std::sin(glfwGetTime())));
    shaders_.at("explode").setFloat("explode_magnitude", explode_magnitude);
    shaders_.at("explode_shadow").setFloat("explode_magnitude", explode_magnitude);

    glViewport(0, 0, ShadowResources::map_size, ShadowResources::map_size);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    const auto &lights = win_data_->lights;
    for (size_t i = 0; i < shadow_.directional_fbos.size(); ++i) {
        shadow_.directional_fbos[i].bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderDirectionalShadowCasters(lights.directional[i].light_space_transform_);
    }
    for (size_t i = 0; i < shadow_.spot_fbos.size(); ++i) {
        shadow_.spot_fbos[i].bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderDirectionalShadowCasters(lights.spot[i].light_space_transform_);
    }
    for (size_t i = 0; i < shadow_.point_fbos.size(); ++i) {
        shadow_.point_fbos[i].bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderOmniShadowCasters(
            shadow_.point_light_spaces[i].data(),
            glm::vec3(lights.point[i].pos_),
            ShadowResources::point_far
        );
    }

    FrameBuffer::unbind();
    glCullFace(GL_BACK);
    RenderObject::invalidateCachedState();
    shadow_.data.apply(shaders_.at("general"), 16);
    shadow_.data.apply(shaders_.at("explode"), 16);
}

void Renderer::renderScene(Camera &cam)
{
    current_render_camera_ = &cam;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (auto &m : objects_) {
        m.render();
    }
    for (auto &m : outline_objects_) {
        m.render();
    }
    auto sorted = sortObjectsByDistance(cam, transparent_objects_);
    for (auto &obj : sorted) {
        obj.first->render();
    }
}

void Renderer::resizeMirrorIfNeeded()
{
    if (!mirror_) {
        return;
    }
    auto &mirror = *mirror_;
    if (mirror.color_ms.width() == win_data_->win_width
        && mirror.color_ms.height() == win_data_->win_height / 2) {
        return;
    }
    const int w = win_data_->win_width;
    const int h = win_data_->win_height / 2;
    mirror.color_ms.reallocate(w, h, GL_RGB, MirrorResources::samples);
    mirror.depth_stencil_rbo.reallocate(GL_DEPTH24_STENCIL8, w, h, MirrorResources::samples);
}

void Renderer::renderMirror()
{
    if (!mirror_) {
        return;
    }
    auto &mirror = *mirror_;
    Camera mirror_camera{ win_data_->camera };
    mirror_camera.yaw(-180);
    mirror_camera.pitch(-2 * mirror_camera.pitch());
    CameraData mirror_cam_data{ win_data_->cam_data };
    mirror_cam_data.view = mirror_camera.viewMatrix();
    mirror_cam_data.projection = glm::perspective(
        glm::radians(win_data_->fov),
        static_cast<float>(mirror.color_ms.width())
            / static_cast<float>(mirror.color_ms.height()),
        0.1f,
        100000.0f
    );
    mirror.fbo.bind();
    glViewport(0, 0, mirror.color_ms.width(), mirror.color_ms.height());
    win_data_->cam_data_UBO.setSubData(0, sizeof(CameraData), &mirror_cam_data);
    renderScene(mirror_camera);
    mirror.fbo.unbind();
}

void Renderer::renderFrame()
{
    resizeMirrorIfNeeded();
    updateShadowMaps();
    renderMirror();

    glViewport(0, 0, win_data_->win_width, win_data_->win_height);
    win_data_->cam_data_UBO.setSubData(0, sizeof(CameraData), &win_data_->cam_data);
    renderScene(win_data_->camera);

    if (mirror_) {
        mirror_->color_ms.bind(0);
        mirror_->quad.render();
    }
}
