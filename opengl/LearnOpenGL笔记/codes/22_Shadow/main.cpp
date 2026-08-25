#include <chrono>
#include <cstddef>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <vector>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Buffers.h"
#include "Material.h"
#include "ShaderProgram.h"
#include "Mesh.h"
#include "glm/detail/func_geometric.hpp"
#include "glm/detail/func_trigonometric.hpp"
#include "Camera.h"
#include "glm/detail/type_mat.hpp"
#include "glm/detail/type_vec.hpp"
#include "RenderObject.h"
#include "Light.h"
#include "Model.h"
#include "FrameBuffer.h"
#include "TextureCubeMap.h"
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include <stb_image.h>
#include <random>
#include "glm/gtc/quaternion.hpp"
#include <array>
#include <filesystem>
#include <format>
#include <unordered_map>

static constexpr int def_win_width = 1920;
static constexpr int def_win_height = 1080;

static std::vector<Model> objects;
static std::vector<Model> outline_objects;
static std::vector<Model> transparent_objects;
static std::unordered_map<std::string, ShaderProgram> shaders;
static Camera *current_render_camera = nullptr;

struct WinData
{
    Camera camera{ Camera::Type::Fly, glm::vec3{ 0.0f, 1.0f, 0.0f } };
    int win_width, win_height;
    LightData lights;
    UniformBuffer lights_UBO{ sizeof(LightData) };
    CameraData cam_data;
    UniformBuffer cam_data_UBO{ sizeof(CameraData) };
    std::chrono::time_point<std::chrono::steady_clock> last_time{ std::chrono::steady_clock::now() };
    std::chrono::duration<float> delta_time{ 0 };
    float fov{ 45.0f };
    bool first_mouse{ true };
    double mouse_x{ 0.0 }, mouse_y{ 0.0 };
    float move_speed{ 20.0f };
    float rotate_sensitivity{ 0.05f };
    float zoom_sensitivity{ 0.1f };
};

void processKey(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return;
    }
    
    WinData *data = static_cast<WinData*>(glfwGetWindowUserPointer(window));
    auto &camera{ data->camera };
    float distance = data->move_speed * data->delta_time.count();
    auto front = camera.front();
    auto right = camera.right();
    if (camera.type() == Camera::Type::FPS) {
        front.y = 0;
        front = glm::normalize(front);
        right.y = 0;
        right = glm::normalize(right);
    }
    glm::vec3 world_up{ 0.0f, 1.0f, 0.0f };
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        auto vec = front * distance;
        camera.move(vec);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        auto vec = -front * distance;
        camera.move(vec);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        auto vec = -right * distance;
        camera.move(vec);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        auto vec = right * distance;
        camera.move(vec);
    }
    if (camera.type() == Camera::Type::Fly && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        auto vec = world_up * distance;
        camera.move(vec);
    }
}

void processMouse(GLFWwindow *window)
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    WinData *data = static_cast<WinData*>(glfwGetWindowUserPointer(window));
    auto &camera{ data->camera };
    if (data->first_mouse) {
        data->first_mouse = false;
        data->mouse_x = x;
        data->mouse_y = y;
        return;
    }

    if (camera.type() == Camera::Type::Fly && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        float distance = data->move_speed * data->delta_time.count();
        auto vec = glm::vec3{ 0.0f, -1.0f, 0.0f } * distance;
        camera.move(vec);
    }

    auto delta_x = x - data->mouse_x;
    auto delta_y = y - data->mouse_y;
    data->mouse_x = x;
    data->mouse_y = y;

    auto delta_yaw = data->rotate_sensitivity * delta_x;
    auto delta_pitch = data->rotate_sensitivity * delta_y;

    data->camera.yaw(-delta_yaw);
    data->camera.pitch(-delta_pitch);
}

void processInput(GLFWwindow *window)
{
    processKey(window);
    processMouse(window);
    
    WinData *data = static_cast<WinData*>(glfwGetWindowUserPointer(window));
    data->cam_data.pos = glm::vec4{ data->camera.pos(), 1.0f };
    data->cam_data.view = data->camera.viewMatrix();
    data->cam_data_UBO.setSubData(0, sizeof(CameraData), &data->cam_data);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    WinData *data = static_cast<WinData*>(glfwGetWindowUserPointer(window));
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto data = static_cast<WinData*>(glfwGetWindowUserPointer(window));
    data->fov -= (float)yoffset;
    if (data->fov < 1.0f)
        data->fov = 1.0f;
    if (data->fov > 45.0f)
        data->fov = 45.0f;
    data->cam_data.projection = glm::perspective(
        glm::radians(data->fov),
        static_cast<float>(data->win_width) / static_cast<float>(data->win_height),
        0.1f,
        100000.0f
    );
    data->cam_data_UBO.setSubData(0, sizeof(CameraData), &data->cam_data);
}

void printOpenGLInfo()
{
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
}

void initGLFW(int major, int minor)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
}

GLFWwindow* createWindow()
{
    GLFWwindow* window = glfwCreateWindow(def_win_width, def_win_height, "GammaCorrection", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwMakeContextCurrent(window);
    return window;
}

void initLightData(LightData &data)
{
    // 1. directional
    data.counts.x = 1;
    data.directional[0].direction_ = glm::normalize(glm::vec4{ -1, -1, -1, 0 });
    data.directional[0].ambient_ = glm::vec4{ 0.05 };
    data.directional[0].diffuse_ = glm::vec4{ 0.3 };
    data.directional[0].specular_ = glm::vec4{ 0.5 };
    data.directional[0].light_space_transform_ = calcDirectionalLightSpaceTransform(
        glm::vec3(data.directional[0].direction_), 20.0f, 1.0f, 100.0f
    );

    // 2. point — 放在 createPlatform 大立方体内部中心
    // platform: center (0,-50,0), half-extent 50 → 内部约 [-50,50]x[-100,0]x[-50,50]
    data.counts.y = 1;
    data.point[0].pos_ = glm::vec4{ 0.0f, -50.0f, 0.0f, 1.0f };
    data.point[0].ambient_ = glm::vec4{ 0.05f };
    data.point[0].diffuse_ = glm::vec4{ 1.0f };
    data.point[0].specular_ = glm::vec4{ 1.0f };
    // 无距离衰减: 1 / (1 + 0*d + 0*d^2) = 1
    data.point[0].attenuation = glm::vec4{ 1.0f, 0.0f, 0.0f, 0.0f };
    data.point[0].light_space_transform_ = glm::mat4{ 1.0f };

    // 3. spot
    data.counts.z = 1;
    data.spot[0].pos_ = glm::vec4{ 0, 10, 10, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[0].pos_));
        // inner/outer: 余弦值，略放宽锥角，避免只有中心一点亮
        data.spot[0].direction_inner_ = glm::vec4{ dir, glm::cos(glm::radians(22.5f)) };
    }
    data.spot[0].ambient_ = glm::vec4{ 0.0 };
    data.spot[0].diffuse_ = glm::vec4{ 0.3 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    // 距离约 14（灯到原点），过强的 quadratic 会把光压得很暗
    data.spot[0].attenuation_outter_ = glm::vec4{ 1.0, 0.0009, 0.00032, glm::cos(glm::radians(37.5f)) };
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
    data.spot[1].attenuation_outter_ = glm::vec4{ 1.0, 0.09, 0.032, glm::cos(glm::radians(17.5f)) };
    data.spot[1].light_space_transform_ = calcSpotLightSpaceTransform(
        glm::vec3(data.spot[1].pos_),
        glm::vec3(data.spot[1].direction_inner_),
        data.spot[1].attenuation_outter_.w,
        0.1f, 50.0f
    );
}

void initWinData(WinData &win_data)
{
    win_data.fov = 45.0f;
    win_data.win_width = def_win_width;
    win_data.win_height = def_win_height;
    win_data.cam_data.pos = glm::vec4{ win_data.camera.pos(), 1.0f };
    win_data.cam_data.view = win_data.camera.viewMatrix();
    win_data.cam_data.projection = glm::perspective(
        glm::radians(win_data.fov),
        static_cast<float>(win_data.win_width) / static_cast<float>(win_data.win_height),
        0.1f,
        100000.0f
    );
    initLightData(win_data.lights);
    
    win_data.lights_UBO.bindBase(0);
    win_data.lights_UBO.setSubData(0, sizeof(LightData), &win_data.lights);
    win_data.cam_data_UBO.bindBase(1);
    win_data.cam_data_UBO.setSubData(0, sizeof(CameraData), &win_data.cam_data);
}

auto initContextAndWindow()
{
    initGLFW(3, 3);
    auto window = createWindow();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error{ "Failed to init GLAD" };
    printOpenGLInfo();
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    auto win_data = std::make_unique<WinData>();
    initWinData(*win_data);

    glfwSetWindowUserPointer(window, win_data.get());
    auto deleter = [data = std::move(win_data)](GLFWwindow* window) {
        if (window) {
            glfwDestroyWindow(window);
        }
    };
    
    return std::unique_ptr<GLFWwindow, decltype(deleter)>{
        window, std::move(deleter)
    };
}

Mesh createQuadMesh(InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} })
{
    static std::vector<Vertex> vertices = {
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
    };
    static std::vector<std::uint32_t> indices = {
        0, 1, 2,
        0, 2, 3,
        0, 2, 1,
        0, 3, 2
    };
    static VertexBuffer vbo{ vertices };
    static IndexBuffer ebo{ indices };

    return Mesh( vbo, ebo, ibo );
}

Mesh createCubeMesh(
    InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} },
    bool include_inner_faces = true)
{
    const float x = 0.5f, y = 0.5f, z = 0.5f;
    static const std::vector<Vertex> vertices_outer = {
        // front
        { { -x, -y, z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { x, -y, z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { x, y, z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
        { { -x, y, z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
        // back
        { { x, -y, -z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }},
        { { -x, -y, -z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }},
        { { -x, y, -z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }},
        { { x, y, -z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }},
        // left
        { { -x, -y, -z }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }},
        { { -x, -y, z }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }},
        { { -x, y, z }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }},
        { { -x, y, -z }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f }},
        // right
        { { x, -y, z }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
        { { x, -y, -z }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }},
        { { x, y, -z }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }},
        { { x, y, z }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }},
        // top
        { { -x, y, z }, { 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
        { { x, y, z }, { 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }},
        { { x, y, -z }, { 1.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }},
        { { -x, y, -z }, { 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }},
        // bottom
        { { x, -y, z }, { 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }},
        { { -x, -y, z }, { 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }},
        { { -x, -y, -z }, { 1.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }},
        { { x, -y, -z }, { 0.0f, 1.0f }, { 0.0f, -1.0f, 0.0f }},
    };
    static const std::vector<std::uint32_t> indices_outer = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23,
    };

    // 内侧面：独立顶点 + 翻转法线；略向中心收缩，避免与外侧共面 z-fighting
    // （否则点光照亮内侧顶面时，会与外侧“地面”打架，整片世界发红）
    static const std::vector<Vertex> vertices_with_inner = [] {
        constexpr float inset = 0.998f;
        std::vector<Vertex> verts = vertices_outer;
        verts.reserve(vertices_outer.size() * 2);
        for (const Vertex &v : vertices_outer) {
            verts.push_back(Vertex{ v.position * inset, v.texCoord, -v.normal });
        }
        return verts;
    }();
    static const std::vector<std::uint32_t> indices_with_inner = [] {
        std::vector<std::uint32_t> idx = indices_outer;
        const std::uint32_t base = static_cast<std::uint32_t>(vertices_outer.size());
        for (size_t i = 0; i + 2 < indices_outer.size(); i += 3) {
            idx.push_back(indices_outer[i] + base);
            idx.push_back(indices_outer[i + 2] + base);
            idx.push_back(indices_outer[i + 1] + base);
        }
        return idx;
    }();

    static VertexBuffer vbo_outer{ vertices_outer };
    static IndexBuffer ebo_outer{ indices_outer };
    static VertexBuffer vbo_with_inner{ vertices_with_inner };
    static IndexBuffer ebo_with_inner{ indices_with_inner };

    if (include_inner_faces) {
        return Mesh{ vbo_with_inner, ebo_with_inner, ibo };
    }
    return Mesh{ vbo_outer, ebo_outer, ibo };
}

RenderObject createGrasses(const std::string &resourceDir)
{
    Material material;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/grass.png"));
    material.diffuse_textures_[0].setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    std::vector<InstanceBuffer::InstanceData> grasses {
        { .translation_ = { 7.0f,  1.0f,  -4.0f } },
        { .translation_ = { 0.0f,  1.0f,  -4.0f } },
        { .translation_ = { -7.0f, 1.0f, -3.0f } },
        { .translation_ = { -7.0f, 1.0f, 0.0f } },
        { .translation_ = { 7.0f,  1.0f,  0.0f } },
        { .translation_ = { -7.0f, 1.0f, 3.0f } },
        { .translation_ = { 0.0f,  1.0f, 3.0f } },
        { .translation_ = { 7.0f,  1.0f,  4.0f } },
    };
    return RenderObject{
        material, createQuadMesh(InstanceBuffer{ std::move(grasses) }),
        shaders.at("general"), shaders.at("shadow"), shaders.at("cube_shadow")
    };
}

std::vector<RenderObject> createGlasses(const std::string &resourceDir)
{
    Material material;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/window.png"));
    material.diffuse_textures_[0].setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    std::vector<InstanceBuffer::InstanceData> instances {
        { .translation_ = { 7.0f,  1.0f,  -5.0f } },
        { .translation_ = { 0.0f,  1.0f,  -5.0f } },
        { .translation_ = { -7.0f, 1.0f, -4.0f } },
        { .translation_ = { -7.0f, 1.0f, 1.0f } },
        { .translation_ = { 7.0f,  1.0f,  1.0f } },
        { .translation_ = { -7.0f, 1.0f, 4.0f } },
        { .translation_ = { 0.0f,  1.0f, 4.0f } },
        { .translation_ = { 7.0f,  1.0f,  5.0f } },
    };
    std::vector<RenderObject> glasses;
    glasses.reserve(instances.size());
    for (auto &instance : instances) {
        glasses.push_back(RenderObject{
            material, createQuadMesh(instance),
            shaders.at("general"), shaders.at("shadow"), shaders.at("cube_shadow")
        });
        glasses.back().render_state_.blend_ = true;
    }
    return glasses;
}

RenderObject createPlatform(const std::string &resourceDir)
{
    Material material;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/container2.png"));
    material.specular_textures_.push_back(Texture2D(resourceDir + "/textures/container2_specular.png"));
    material.shininess_ = 1.0f;

    InstanceBuffer::InstanceData platform;
    platform.translation_ = glm::vec3{ 0.0f, -50.0f, 0.0f };
    platform.scale_ = glm::vec3{ 100.0f };

    return RenderObject{
        material, createCubeMesh(platform),
        shaders.at("general"), shaders.at("shadow"), shaders.at("cube_shadow")
    };
}

// 在 createPlatform 大立方体内部：若干小方块 + 点光源位置的纯色标记
void addPlatformInterior(const std::string &resourceDir, const LightData &lights)
{
    Material material;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/container2.png"));
    material.specular_textures_.push_back(Texture2D(resourceDir + "/textures/container2_specular.png"));
    material.shininess_ = 64.0f;

    // 相对 platform 中心 (0, -50, 0) 摆放；偏外侧，靠近内壁
    const glm::vec3 room_center{ 0.0f, -50.0f, 0.0f };
    std::vector<InstanceBuffer::InstanceData> cubes {
        { .translation_ = room_center + glm::vec3{  35.0f, -5.0f,  0.0f }, .scale_ = { 2.0f, 2.0f, 2.0f } },
        { .translation_ = room_center + glm::vec3{ -38.0f,  8.0f,  20.0f }, .scale_ = { 1.5f, 3.0f, 1.5f } },
        { .translation_ = room_center + glm::vec3{  25.0f,-30.0f, -35.0f }, .scale_ = { 2.5f, 1.5f, 2.5f } },
        { .translation_ = room_center + glm::vec3{ -30.0f,-20.0f, -38.0f }, .scale_ = { 1.0f, 1.0f, 4.0f } },
        { .translation_ = room_center + glm::vec3{   0.0f,-40.0f,  36.0f }, .scale_ = { 3.0f, 1.0f, 3.0f } },
        { .translation_ = room_center + glm::vec3{  40.0f,  0.0f,  32.0f }, .scale_ = { 1.2f, 1.2f, 1.2f } },
    };
    for (auto &cube : cubes) {
        objects.push_back(RenderObject{
            material, createCubeMesh(cube, false),
            shaders.at("general"), shaders.at("shadow"), shaders.at("cube_shadow")
        });
    }

    for (int i = 0; i < lights.counts.y; ++i) {
        Material marker_mat;
        marker_mat.pure_color_ = true;
        marker_mat.color_ = glm::vec3{ 1.0f };
        InstanceBuffer::InstanceData marker;
        marker.translation_ = glm::vec3(lights.point[i].pos_);
        marker.scale_ = glm::vec3{ 0.5f };
        objects.push_back(RenderObject{
            marker_mat, createCubeMesh(marker, false),
            shaders.at("general"), ShaderProgram{}, ShaderProgram{}
        });
    }
}

std::vector<RenderObject> createCubes(const std::string &resourceDir)
{
    Material material;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/container2.png"));
    material.specular_textures_.push_back(Texture2D(resourceDir + "/textures/container2_specular.png"));
    material.shininess_ = 128.0f;

    std::vector<InstanceBuffer::InstanceData> cubes {
        { .translation_ = { 0.0f,  1.0f,  0.0f }, .scale_ = { 2, 2, 2 } },
        { .translation_ = { 10.0f,  0.5f,  0.0f } },
        { .translation_ = { 10.0f,  1.7f,  0.0f } }
    };

    std::vector<RenderObject> result;
    for (auto &cube : cubes) {
        result.emplace_back(
            material, createCubeMesh(cube),
            shaders.at("general"), shaders.at("shadow"), shaders.at("cube_shadow")
        );
    }
    return result;
}

static void setupStencilMaskWriter(RenderObject &obj, GLint ref = 1)
{
    obj.render_state_.stencil_test_ = true;
    obj.render_state_.stencil_func_ = GL_ALWAYS;
    obj.render_state_.stencil_ref_ = ref;
    obj.render_state_.stencil_mask_ = 0xff;
    obj.render_state_.stencil_sfail_ = GL_KEEP;
    obj.render_state_.stencil_dpfail_ = GL_KEEP;
    obj.render_state_.stencil_dppass_ = GL_REPLACE;
    obj.render_state_.stencil_write_mask_ = 0xff;
}

static RenderObject createOutlineShell(const RenderObject &src, const glm::vec3 &color, float scale = 1.2f)
{
    RenderObject outline{ src };
    outline.material_.pure_color_ = true;
    outline.material_.color_ = color;
    outline.directional_shadow_shader_ = ShaderProgram{};
    outline.omni_shadow_shader_ = ShaderProgram{};
    auto instances = outline.mesh_.instanceBuffer().data();
    for (auto &instance : instances) {
        instance.scale_ *= glm::vec3{ scale };
    }
    outline.mesh_.setInstanceBuffer(std::move(instances));

    outline.render_state_.stencil_test_ = true;
    outline.render_state_.stencil_func_ = GL_NOTEQUAL;
    outline.render_state_.stencil_ref_ = 1;
    outline.render_state_.stencil_mask_ = 0xff;
    outline.render_state_.stencil_sfail_ = GL_KEEP;
    outline.render_state_.stencil_dpfail_ = GL_KEEP;
    outline.render_state_.stencil_dppass_ = GL_KEEP;
    outline.render_state_.stencil_write_mask_ = 0x00;
    outline.render_state_.depth_func_ = GL_ALWAYS;
    return outline;
}

std::vector<RenderObject> createOutlineCubes(
    const std::string &resourceDir,
    [[maybe_unused]] const glm::vec3 &outline_color = glm::vec3{ 0.0f, 1.0f, 0.0f })
{
    auto cubes = createCubes(resourceDir);
    std::vector<RenderObject> result;
    result.reserve(cubes.size() * 2);
    for (auto &cube : cubes) {
        setupStencilMaskWriter(cube);
        result.push_back(cube);
        //result.push_back(createOutlineShell(cube, outline_color));
    }
    return result;
}

std::vector<std::pair<Model*, float>> sortObjectsByDistance(Camera &camera, const std::vector<Model> &objs)
{
    auto cam_pos = camera.pos();
    std::vector<std::pair<Model*, float>> sorted;
    sorted.reserve(objs.size());
    for (auto &obj : objs) {
        sorted.push_back({
            const_cast<Model*>(&obj),
            glm::distance(cam_pos, obj.objects_[0].mesh_.instanceBuffer().data().front().translation_)
        });
    }
    std::sort(sorted.begin(), sorted.end(), [cam_pos](auto &a, auto &b) {
        return a.second > b.second;
    });
    return sorted;
}

void createShaders(const std::string &resourceDir)
{
    auto add = [&](const std::string &name,
                   const std::filesystem::path &vert,
                   const std::filesystem::path &frag,
                   const std::filesystem::path &geom = {}) {
        shaders.emplace(name, ShaderProgram{ vert, frag, geom });
    };

    add("general",
        resourceDir + "shaders/generaL_2.vert",
        resourceDir + "shaders/generaL_2.frag");
    shaders.at("general").setUniformBlockBinding("LightData", 0);
    shaders.at("general").setUniformBlockBinding("CamData", 1);

    add("shadow",
        resourceDir + "shaders/shadow_0.vert",
        resourceDir + "shaders/shadow_0.frag");

    add("cube_shadow",
        resourceDir + "shaders/cube_shadow_0.vert",
        resourceDir + "shaders/cube_shadow_0.frag",
        resourceDir + "shaders/cube_shadow_0.geom");

    add("visual_normal",
        resourceDir + "shaders/visual_normal_0.vert",
        resourceDir + "shaders/visual_normal_0.frag",
        resourceDir + "shaders/visual_normal_0.geom");
    shaders.at("visual_normal").setUniformBlockBinding("CamData", 1);
    shaders.at("visual_normal").setVec3("normal_color", glm::vec3{ 0, 1, 0 });

    add("explode",
        resourceDir + "shaders/explode_0.vert",
        resourceDir + "shaders/explode_0.frag",
        resourceDir + "shaders/explode_0.geom");
    shaders.at("explode").setUniformBlockBinding("LightData", 0);
    shaders.at("explode").setUniformBlockBinding("CamData", 1);

    add("explode_shadow",
        resourceDir + "shaders/shadow_explode_0.vert",
        resourceDir + "shaders/shadow_explode_0.frag",
        resourceDir + "shaders/shadow_explode_0.geom");

    add("skybox",
        resourceDir + "shaders/skybox_0.vert",
        resourceDir + "shaders/skybox_0.frag");
    shaders.at("skybox").setUniformBlockBinding("CamData", 1);
    shaders.at("skybox").setInt("skybox", 0);

    add("kernel",
        resourceDir + "shaders/kernel_1.vert",
        resourceDir + "shaders/kernel_1.frag");
}

void createScene(const std::string &resourceDir, const LightData &lights)
{
    auto &general = shaders.at("general");
    auto &shadow = shaders.at("shadow");
    auto &cube_shadow = shaders.at("cube_shadow");

    outline_objects.push_back(createOutlineCubes(resourceDir));
    objects.push_back(createPlatform(resourceDir));
    addPlatformInterior(resourceDir, lights);

    Model flashlight{ resourceDir + "/model/flash_light", "Flashlight.obj",
                      general, shadow, cube_shadow, false };
    for (int i = 0; i < lights.counts.z; ++i) {
        auto &spot{ lights.spot[i] };
        InstanceBuffer::InstanceData instance;
        instance.translation_ = spot.pos_;

        glm::vec3 direction{ spot.direction_inner_ };
        float yaw = atan2(direction.x, direction.z);
        float horizontal = std::hypot(direction.x, direction.z);
        float pitch = atan2(-direction.y, horizontal);
        instance.rotation_ = glm::angleAxis(yaw, glm::vec3{ 0, 1, 0 })
            * glm::angleAxis(pitch, glm::vec3{ 1, 0, 0 });
        instance.scale_ = glm::vec3{ 0.3f };

        objects.push_back(flashlight);
        objects.back().setInstances(InstanceBuffer{ instance });
        // 灯具在光源位置，若参与 shadow pass 会挡住整锥，导致场景全黑
        objects.back().setShadowShaders(ShaderProgram{}, ShaderProgram{});
    }

    Model backpack{ resourceDir + "/model/backpack", "backpack.obj",
                    general, shadow, cube_shadow };
    backpack.setInstances(InstanceBuffer::InstanceData{ .translation_ = { -5, 2, 0 } });
    objects.push_back(backpack);

    backpack.setInstances(InstanceBuffer::InstanceData{ .translation_ = { -5, 2, -5 } });
    objects.push_back(backpack);

    backpack.setRenderShader(shaders.at("visual_normal"));
    objects.push_back(backpack);

    backpack.setRenderShader(shaders.at("explode"));
    backpack.setDirectionalShadowShader(shaders.at("explode_shadow"));
    backpack.setInstances(InstanceBuffer::InstanceData{ .translation_ = { 0, 4, -5 } });
    objects.push_back(backpack);

    const glm::vec3 planet_pos{ 10, 30, 0 };
    objects.emplace_back(resourceDir + "/model/planet", "planet.obj",
                         general, shadow, cube_shadow);
    objects.back().setInstances(InstanceBuffer::InstanceData{ .translation_ = planet_pos });

    // objects.emplace_back(resourceDir + "/model/rock", "rock.obj", general, shadow);
    // std::vector<InstanceBuffer::InstanceData> rocks;
    // const glm::vec3 rock_offset{ 25, 0, 0 };
    // const glm::vec3 planet_axis = glm::normalize(glm::vec3{ 0, 1, 0.5 });
    // std::mt19937 rand_gen{ std::random_device{}() };
    // std::normal_distribution<float> normal_dist{ 0, 1 };
    // for (int i = 0; i < 10000; ++i) {
    //     glm::vec3 rand_offset{ normal_dist(rand_gen), normal_dist(rand_gen), normal_dist(rand_gen) };
    //     float orbit_angle = 360.0f / 1000.0f * i;

    //     glm::quat orbit_rotation = glm::angleAxis(glm::radians(orbit_angle), planet_axis);
    //     glm::quat local_rotation = glm::angleAxis(normal_dist(rand_gen), glm::normalize(rand_offset));
    //     InstanceBuffer::InstanceData instance;
    //     instance.translation_ = planet_pos + orbit_rotation * (rock_offset + rand_offset);
    //     instance.rotation_ = orbit_rotation * local_rotation;
    //     instance.scale_ = glm::vec3{ 0.1 };
    //     rocks.push_back(instance);
    // }
    // objects.back().setInstances(std::move(rocks));

    objects.push_back(createGrasses(resourceDir));

    RenderObject skybox_obj{ Material{}, createCubeMesh() };
    skybox_obj.render_shader_ = shaders.at("skybox");
    skybox_obj.render_state_.cull_face_ = false;
    skybox_obj.render_state_.depth_func_ = GL_LEQUAL;
    skybox_obj.material_.diffuse_textures_.push_back(TextureCubeMap{
        {
            resourceDir + "/textures/skybox/right.jpg",
            resourceDir + "/textures/skybox/left.jpg",
            resourceDir + "/textures/skybox/top.jpg",
            resourceDir + "/textures/skybox/bottom.jpg",
            resourceDir + "/textures/skybox/front.jpg",
            resourceDir + "/textures/skybox/back.jpg"
        },
        false
    });
    skybox_obj.action_ = [](ShaderProgram &shader, RenderPass pass) {
        if (pass != RenderPass::Draw || !current_render_camera) {
            return;
        }
        shader.setMat4(
            "no_translate_view",
            glm::mat4(glm::mat3(current_render_camera->viewMatrix()))
        );
    };
    objects.push_back(std::move(skybox_obj));

    auto glasses = createGlasses(resourceDir);
    for (auto &g : glasses)
        transparent_objects.push_back(std::move(g));
}

struct ShadowResources {
    static constexpr int map_size = 2048;
    static constexpr float point_near = 0.1f;
    static constexpr float point_far = 100.0f; // 覆盖大立方体内部对角线约 86
    ShadowData data;
    std::vector<FrameBuffer> directional_fbos;
    std::vector<FrameBuffer> spot_fbos;
    std::vector<FrameBuffer> point_fbos;
    // 与 point_fbos 一一对应：每个点光源的 6 面 light-space 矩阵
    std::vector<std::array<glm::mat4, 6>> point_light_spaces;
};

ShadowResources createShadowResources(const LightData &lights)
{
    ShadowResources res;

    for (int i = 0; i < lights.counts.x; ++i) {
        res.data.directional[i] = { createShadowMapTexture(ShadowResources::map_size), 0.00001f };
        res.directional_fbos.push_back(createShadowMapFBO(res.data.directional[i].first));
    }
    res.data.counts.x = lights.counts.x;

    for (int i = 0; i < lights.counts.y; ++i) {
        res.data.point[i] = { createShadowMapTextureCube(ShadowResources::map_size), 0.15f };
        res.data.point_far[i] = ShadowResources::point_far;
        // 整张 cubemap 挂到 FBO（layered），配合 cube_shadow geometry shader 一次写 6 面
        res.point_fbos.push_back(createShadowMapFBO(res.data.point[i].first));
        res.point_light_spaces.push_back(calcPointLightSpaceTransforms(
            glm::vec3(lights.point[i].pos_),
            ShadowResources::point_near,
            ShadowResources::point_far
        ));
    }
    res.data.counts.y = lights.counts.y;

    for (int i = 0; i < lights.counts.z; ++i) {
        res.data.spot[i] = { createShadowMapTexture(ShadowResources::map_size), 0.00001f };
        res.spot_fbos.push_back(createShadowMapFBO(res.data.spot[i].first));
    }
    res.data.counts.z = lights.counts.z;
    return res;
}

struct MirrorResources {
    static constexpr GLsizei samples = 4;
    Texture2DMS color_ms;
    RenderBufferMS depth_stencil_rbo;
    FrameBuffer fbo;
    RenderObject quad;
};

MirrorResources createMirror(int width, int height)
{
    const int mirror_h = height / 2;
    MirrorResources mirror{
        Texture2DMS{ width, mirror_h, GL_RGB, MirrorResources::samples },
        RenderBufferMS{ GL_DEPTH24_STENCIL8, width, mirror_h, MirrorResources::samples },
        FrameBuffer{},
        RenderObject{}
    };

    mirror.fbo.attachTexture(GL_COLOR_ATTACHMENT0, mirror.color_ms);
    mirror.fbo.attachRBO(GL_DEPTH_STENCIL_ATTACHMENT, mirror.depth_stencil_rbo);
    if (!mirror.fbo.isCompleted()) {
        throw std::runtime_error{ "Mirror FBO is not completed" };
    }

    mirror.quad.render_shader_ = shaders.at("kernel");
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
    return mirror;
}

void renderDirectionalShadowCasters(const glm::mat4 &light_space)
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
    for (auto &m : objects) {
        draw_model_shadow(m);
    }
    for (auto &m : outline_objects) {
        draw_model_shadow(m);
    }
}

// 点光源 / 万向光：各物体用自己的 omni_shadow_shader_（cube_shadow_0 + GS）一次写 cubemap 6 面
void renderOmniShadowCasters(const glm::mat4 *light_spaces,
                             const glm::vec3 &light_pos,
                             float far_plane)
{
    auto draw_model_shadow = [&](Model &m) {
        for (auto &obj : m.objects_) {
            if (obj.omni_shadow_shader_.id() == 0) {
                continue;
            }
            auto &shader = obj.omni_shadow_shader_;
            shader.setMat4Arr("light_space_transform", light_spaces, 6);
            shader.setVec3("light_pos", light_pos);
            shader.setFloat("far_plane", far_plane);
            obj.renderOmniShadow();
        }
    };
    for (auto &m : objects) {
        draw_model_shadow(m);
    }
    for (auto &m : outline_objects) {
        draw_model_shadow(m);
    }
}

void updateShadowMaps(ShadowResources &shadow, const LightData &lights)
{
    const float explode_magnitude = std::abs(static_cast<float>(sin(glfwGetTime())));
    shaders.at("explode").setFloat("explode_magnitude", explode_magnitude);
    shaders.at("explode_shadow").setFloat("explode_magnitude", explode_magnitude);

    glViewport(0, 0, ShadowResources::map_size, ShadowResources::map_size);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    for (size_t i = 0; i < shadow.directional_fbos.size(); ++i) {
        shadow.directional_fbos[i].bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderDirectionalShadowCasters(lights.directional[i].light_space_transform_);
    }

    for (size_t i = 0; i < shadow.spot_fbos.size(); ++i) {
        shadow.spot_fbos[i].bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderDirectionalShadowCasters(lights.spot[i].light_space_transform_);
    }

    for (size_t i = 0; i < shadow.point_fbos.size(); ++i) {
        shadow.point_fbos[i].bind();
        glClear(GL_DEPTH_BUFFER_BIT);
        renderOmniShadowCasters(
            shadow.point_light_spaces[i].data(),
            glm::vec3(lights.point[i].pos_),
            ShadowResources::point_far
        );
    }

    FrameBuffer::unbind();
    glCullFace(GL_BACK);
    RenderObject::invalidateCachedState();

    shadow.data.apply(shaders.at("general"), 16);
}

void renderScene(Camera &cam)
{
    current_render_camera = &cam;
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (auto &m : objects) {
        m.render();
    }
    for (auto &m : outline_objects) {
        m.render();
    }

    auto sorted = sortObjectsByDistance(cam, transparent_objects);
    for (auto &obj : sorted) {
        obj.first->render();
    }
}

void resizeMirrorIfNeeded(MirrorResources &mirror, int win_width, int win_height)
{
    if (mirror.color_ms.width() == win_width
        && mirror.color_ms.height() == win_height / 2) {
        return;
    }
    const int w = win_width;
    const int h = win_height / 2;
    mirror.color_ms.reallocate(w, h, GL_RGB, MirrorResources::samples);
    mirror.depth_stencil_rbo.reallocate(GL_DEPTH24_STENCIL8, w, h, MirrorResources::samples);
}

void renderMirror(MirrorResources &mirror, WinData &win_data)
{
    Camera mirror_camera{ win_data.camera };
    mirror_camera.yaw(-180);
    mirror_camera.pitch(-2 * mirror_camera.pitch());
    CameraData mirror_cam_data{ win_data.cam_data };
    mirror_cam_data.view = mirror_camera.viewMatrix();
    mirror_cam_data.projection = glm::perspective(
        glm::radians(win_data.fov),
        static_cast<float>(mirror.color_ms.width())
            / static_cast<float>(mirror.color_ms.height()),
        0.1f,
        100000.0f
    );
    mirror.fbo.bind();
    glViewport(0, 0, mirror.color_ms.width(), mirror.color_ms.height());
    win_data.cam_data_UBO.setSubData(0, sizeof(CameraData), &mirror_cam_data);
    renderScene(mirror_camera);
    mirror.fbo.unbind();
}

int main(int argc, char* argv[])
try 
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <resources dir>" << std::endl;
        return -1;
    }
    std::string resourceDir{ argv[1] };

    auto window = initContextAndWindow();
    WinData *win_data = static_cast<WinData*>(glfwGetWindowUserPointer(window.get()));

    createShaders(resourceDir);
    createScene(resourceDir, win_data->lights);
    auto shadow_res = createShadowResources(win_data->lights);
    auto mirror = createMirror(win_data->win_width, win_data->win_height);

    win_data->last_time = steady_clock::now();
    for (unsigned frame = 0; !glfwWindowShouldClose(window.get()); ++frame) {
        resizeMirrorIfNeeded(mirror, win_data->win_width, win_data->win_height);
        updateShadowMaps(shadow_res, win_data->lights);
        renderMirror(mirror, *win_data);

        glViewport(0, 0, win_data->win_width, win_data->win_height);
        win_data->cam_data_UBO.setSubData(0, sizeof(CameraData), &win_data->cam_data);
        renderScene(win_data->camera);

        mirror.color_ms.bind(0);
        mirror.quad.render();

        auto now = steady_clock::now();
        win_data->delta_time = now - win_data->last_time;
        win_data->last_time = now;
        glfwSwapBuffers(window.get());
        glfwPollEvents();
        processInput(window.get());
    }
    // ��̬ Model/Shader ���� GL ��Դ����������������ǰ�ͷ�
    objects.clear();
    outline_objects.clear();
    transparent_objects.clear();
    shaders.clear();
    window.reset();
    glfwTerminate();
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    glfwTerminate();
    return -1;
}
