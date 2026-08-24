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
#include <format>

static constexpr int def_win_width = 1920;
static constexpr int def_win_height = 1080;

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

void initLightData(WinData &win_data)
{
    LightData &data{ win_data.lights };

    // 1. directional
    data.counts.x = 1;
    data.directional[0].direction_ = glm::normalize(glm::vec4{ -1, -1, -1, 0 });
    data.directional[0].ambient_ = glm::vec4{ 0.1 };
    data.directional[0].diffuse_ = glm::vec4{ 0.6 };
    data.directional[0].specular_ = glm::vec4{ 0.9 };
    data.directional[0].light_space_transform_ = calcDirectionalLightSpaceTransform(
        glm::vec3(data.directional[0].direction_), 1.5f, 1.0f, 500.0f
    );

    // 2. point
    data.counts.y = 0;
    data.point[0].pos_ = glm::vec4{ 5, 0, 0, 1 };
    data.point[0].ambient_ = glm::vec4{ 0.2 };
    data.point[0].diffuse_ = glm::vec4{ 0.5 };
    data.point[0].specular_ = glm::vec4{ 1.0 };
    data.point[0].attenuation = glm::vec4{ 1.0, 0.027, 0.0028, 0.0 };
    {
        const glm::vec3 pos = glm::vec3(data.point[0].pos_);
        const glm::vec3 dir = glm::normalize(glm::vec3{ 0.0f } - pos);
        data.point[0].light_space_transform_ = calcSpotLightSpaceTransform(
            pos, dir, glm::cos(glm::radians(45.0f)), 0.1f, 50.0f
        );
    }

    // 3. spot
    data.counts.z = 0;
    data.spot[0].pos_ = glm::vec4{ 0, 10, 10, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[0].pos_));
        data.spot[0].direction_inner_ = glm::vec4{ dir, 0.99 };
    }
    data.spot[0].ambient_ = glm::vec4{ 0.2 };
    data.spot[0].diffuse_ = glm::vec4{ 0.8 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    data.spot[0].attenuation_outter_ = glm::vec4{ 1.0, 0.027, 0.0028, 0.95 };
    data.spot[0].light_space_transform_ = calcSpotLightSpaceTransform(
        glm::vec3(data.spot[0].pos_),
        glm::vec3(data.spot[0].direction_inner_),
        data.spot[0].attenuation_outter_.w,
        0.1f, 50.0f
    );

    data.spot[1].pos_ = glm::vec4{ 10, 10, 0, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[1].pos_));
        data.spot[1].direction_inner_ = glm::vec4{ dir, 0.99 };
    }
    data.spot[1].ambient_ = glm::vec4{ 0.2 };
    data.spot[1].diffuse_ = glm::vec4{ 0.8 };
    data.spot[1].specular_ = glm::vec4{ 1.0 };
    data.spot[1].attenuation_outter_ = glm::vec4{ 1.0, 0.027, 0.0028, 0.95 };
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
    initLightData(win_data);
    
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

Mesh createCubeMesh(InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} })
{
    const float x = 0.5f, y = 0.5f, z = 0.5f;
    static std::vector<Vertex> vertices = {
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
    static std::vector<std::uint32_t> indices = {
        // front
        0, 1, 2,
        0, 2, 3,
        // left
        4, 5, 6,
        4, 6, 7,
        // right
        8, 9, 10,
        8, 10, 11,
        // back
        12, 13, 14,
        12, 14, 15,
        // top
        16, 17, 18,
        16, 18, 19,
        // bottom
        20, 21, 22,
        20, 22, 23,
    };
    static VertexBuffer vbo{ vertices };
    static IndexBuffer ebo{ indices };

    return Mesh{ vbo, ebo, ibo };
}

RenderObject createGrasses(const std::string &resourceDir,
                           ShaderProgram render_shader, ShaderProgram shadow_shader)
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
        render_shader, shadow_shader
    };
}

std::vector<RenderObject> createGlasses(const std::string &resourceDir,
                                        ShaderProgram render_shader)
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
            material, createQuadMesh(instance), render_shader, ShaderProgram{}
        });
        glasses.back().render_state_.blend_ = true;
    }
    return glasses;
}

RenderObject createPlatform(const std::string &resourceDir,
                            ShaderProgram render_shader, ShaderProgram shadow_shader)
{
    Material material;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/container2.png"));
    material.specular_textures_.push_back(Texture2D(resourceDir + "/textures/container2_specular.png"));
    material.shininess_ = 1.0f;

    InstanceBuffer::InstanceData platform;
    platform.translation_ = glm::vec3{ 0.0f, -50.0f, 0.0f };
    platform.scale_ = glm::vec3{ 100.0f };

    return RenderObject{ material, createCubeMesh(platform), render_shader, shadow_shader };
}

std::vector<RenderObject> createCubes(const std::string &resourceDir,
                                      ShaderProgram render_shader, ShaderProgram shadow_shader)
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

    std::vector<RenderObject> objects;
    for (auto &cube : cubes) {
        objects.emplace_back(material, createCubeMesh(cube), render_shader, shadow_shader);
    }
    return objects;
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
    outline.shadow_shader_ = ShaderProgram{};
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

std::vector<RenderObject> createOutlineCubes(const std::string &resourceDir,
                                            ShaderProgram render_shader,
                                            ShaderProgram shadow_shader,
                                            const glm::vec3 &outline_color = glm::vec3{ 0.0f, 1.0f, 0.0f })
{
    auto cubes = createCubes(resourceDir, render_shader, shadow_shader);
    std::vector<RenderObject> result;
    result.reserve(cubes.size() * 2);
    for (auto &cube : cubes) {
        setupStencilMaskWriter(cube);
        result.push_back(cube);
        result.push_back(createOutlineShell(cube, outline_color));
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

Texture2D createShadowMapTexture(int size)
{
    Texture2D tex{ size, size, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, nullptr };
    tex.bind(0);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border_color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    return tex;
}

FrameBuffer createShadowMapFBO(const Texture2D &depth_texture)
{
    FrameBuffer fbo;
    fbo.attachTexture(GL_DEPTH_ATTACHMENT, depth_texture);
    if (!fbo.isCompleted()) {
        throw std::runtime_error{ "Shadow FBO is not completed" };
    }
    return fbo;
}

void bindShadowMapSamplers(
    ShaderProgram &shader,
    const LightData &lights,
    const std::array<Texture2D, 2> &directional_maps,
    const std::array<Texture2D, 4> &spot_maps,
    const Texture2D &dummy_map,
    unsigned first_unit)
{
    unsigned unit = first_unit;
    for (int i = 0; i < 2; ++i) {
        const Texture2D &map = i < lights.counts.x ? directional_maps[i] : dummy_map;
        map.bind(unit);
        shader.setInt(std::format("shadowMap.directional[{}]", i), static_cast<int>(unit));
        ++unit;
    }
    for (int i = 0; i < 4; ++i) {
        const Texture2D &map = i < lights.counts.z ? spot_maps[i] : dummy_map;
        map.bind(unit);
        shader.setInt(std::format("shadowMap.spot[{}]", i), static_cast<int>(unit));
        ++unit;
    }
    for (int i = 0; i < 8; ++i) {
        dummy_map.bind(unit);
        shader.setInt(std::format("shadowMap.point_light[{}]", i), static_cast<int>(unit));
        ++unit;
    }
}

int main(int argc, char* argv[])
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    try {
        if (argc != 2) {
            std::cout << "Usage: " << argv[0] << " <resources dir>" << std::endl;
            return -1;
        }
        std::string resourceDir{ argv[1] };
    
        auto window = initContextAndWindow();
        WinData *win_data = static_cast<WinData*>(glfwGetWindowUserPointer(window.get()));


        std::vector<Model> objects;
        std::vector<Model> outline_objects;
        std::vector<Model> transparent_objects;

        ShaderProgram general_shader{
            resourceDir + "shaders/generaL_2.vert",
            resourceDir + "shaders/generaL_2.frag"
        };
        general_shader.setUniformBlockBinding("LightData", 0);
        general_shader.setUniformBlockBinding("CamData", 1);

        ShaderProgram shadow_shader{
            resourceDir + "shaders/shadow_0.vert",
            resourceDir + "shaders/shadow_0.frag"
        };

        constexpr int shadow_map_size = 4096;
        Texture2D dummy_shadow_map = createShadowMapTexture(1);
        std::array<Texture2D, 2> directional_shadow_maps{
            createShadowMapTexture(shadow_map_size),
            createShadowMapTexture(shadow_map_size)
        };
        std::array<FrameBuffer, 2> directional_shadow_fbos{
            createShadowMapFBO(directional_shadow_maps[0]),
            createShadowMapFBO(directional_shadow_maps[1])
        };
        std::array<Texture2D, 4> spot_shadow_maps{
            createShadowMapTexture(shadow_map_size),
            createShadowMapTexture(shadow_map_size),
            createShadowMapTexture(shadow_map_size),
            createShadowMapTexture(shadow_map_size)
        };
        std::array<FrameBuffer, 4> spot_shadow_fbos{
            createShadowMapFBO(spot_shadow_maps[0]),
            createShadowMapFBO(spot_shadow_maps[1]),
            createShadowMapFBO(spot_shadow_maps[2]),
            createShadowMapFBO(spot_shadow_maps[3])
        };

        ShaderProgram visual_normal_shader{
            resourceDir + "shaders/visual_normal_0.vert",
            resourceDir + "shaders/visual_normal_0.frag",
            resourceDir + "shaders/visual_normal_0.geom"
        };
        visual_normal_shader.setUniformBlockBinding("CamData", 1);
        visual_normal_shader.setVec3("normal_color", glm::vec3{ 0, 1, 0 });

        ShaderProgram explode_shader {
            resourceDir + "shaders/explode_0.vert",
            resourceDir + "shaders/explode_0.frag",
            resourceDir + "shaders/explode_0.geom"
        };
        explode_shader.setUniformBlockBinding("LightData", 0);
        explode_shader.setUniformBlockBinding("CamData", 1);

        ShaderProgram explode_shadow_shader{
            resourceDir + "shaders/shadow_explode_0.vert",
            resourceDir + "shaders/shadow_explode_0.frag",
            resourceDir + "shaders/shadow_explode_0.geom"
        };
        
        ShaderProgram skybox_shader{
            resourceDir + "shaders/skybox_0.vert",
            resourceDir + "shaders/skybox_0.frag"
        };
        skybox_shader.setUniformBlockBinding("CamData", 1);
        skybox_shader.setInt("skybox", 0);

        outline_objects.push_back(createOutlineCubes(resourceDir, general_shader, shadow_shader));
        objects.push_back(createPlatform(resourceDir, general_shader, shadow_shader));

        Model flashlight{ resourceDir + "/model/flash_light", "Flashlight.obj",
                          general_shader, shadow_shader, false };
        for (int i = 0; i < win_data->lights.counts.z; ++i) {
            auto &spot{ win_data->lights.spot[i] };
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
        }

        objects.emplace_back(resourceDir + "/model/backpack", "backpack.obj",
                             general_shader, shadow_shader);
        objects.back().setInstances(InstanceBuffer::InstanceData{ .translation_ = { -5, 2, 0 } });

        objects.push_back(objects.back());
        objects.back().setInstances(InstanceBuffer::InstanceData{ .translation_ = { -5, 2, -5 } });

        objects.push_back(objects.back());
        objects.back().setRenderShader(visual_normal_shader);

        objects.push_back(objects.back());
        objects.back().setRenderShader(explode_shader);
        objects.back().setShadowShader(explode_shadow_shader);
        objects.back().setInstances(InstanceBuffer::InstanceData{ .translation_ = { 0, 4, -5 } });
        
        glm::vec3 planet_pos{ 10, 30, 0 };
        objects.emplace_back(resourceDir + "/model/planet", "planet.obj",
                             general_shader, shadow_shader);
        objects.back().setInstances(InstanceBuffer::InstanceData{ .translation_ = planet_pos });

        // objects.emplace_back(resourceDir + "/model/rock", "rock.obj", general_shader, shadow_shader);
        // std::vector<InstanceBuffer::InstanceData> rocks;
        // glm::vec3 rock_offset{ 25, 0, 0 };
        // glm::vec3 planet_axis = glm::normalize(glm::vec3{ 0, 1, 0.5 });
        // std::vector<glm::mat4> rock_transforms;
        // std::mt19937 rand_gen{ std::random_device{}() };
        // std::normal_distribution<float> normal_dist{ 0, 1 };
        // for (int i = 0; i < 10000; ++i) {
        //     glm::vec3 rand_offset{ normal_dist(rand_gen), normal_dist(rand_gen), normal_dist(rand_gen) };
        //     float orbit_angle = 360.0f / 1000.0f * i;

        //     glm::quat orbit_rotation = glm::angleAxis(glm::radians(orbit_angle),
        //         glm::normalize(planet_axis));
        //     glm::quat local_rotation = glm::angleAxis(normal_dist(rand_gen),
        //         glm::normalize(rand_offset));
        //     InstanceBuffer::InstanceData instance;
        //     instance.translation_ = planet_pos +
        //         orbit_rotation * (rock_offset + rand_offset);
        //     instance.rotation_ = orbit_rotation * local_rotation;
        //     instance.scale_ = glm::vec3{ 0.1 };
        //     rocks.push_back(instance);
        // }
        // objects.back().setInstances(std::move(rocks));

        objects.push_back(createGrasses(resourceDir, general_shader, shadow_shader));

        RenderObject skybox{ Material{}, createCubeMesh() };
        skybox.render_shader_ = skybox_shader;
        skybox.render_state_.cull_face_ = false;
        skybox.render_state_.depth_func_ = GL_LEQUAL;
        std::vector<std::string> faces
        {
            resourceDir + "/textures/skybox/right.jpg",
            resourceDir + "/textures/skybox/left.jpg",
            resourceDir + "/textures/skybox/top.jpg",
            resourceDir + "/textures/skybox/bottom.jpg",
            resourceDir + "/textures/skybox/front.jpg",
            resourceDir + "/textures/skybox/back.jpg"
        };
        TextureCubeMap cubemap_texture{ faces, false };
        skybox.material_.diffuse_textures_.push_back(cubemap_texture);

        // auto glasses = createGlasses(resourceDir, general_shader);
        // transparent_objects.insert(transparent_objects.end(),
        //     std::make_move_iterator(glasses.begin()),
        //     std::make_move_iterator(glasses.end()));

        constexpr GLsizei mirror_samples = 4;
        const int mirror_w = win_data->win_width;
        const int mirror_h = win_data->win_height / 2;
        Texture2DMS mirror_ms_texture{ mirror_w, mirror_h, GL_RGB, mirror_samples };
        RenderBufferMS mirror_rbo{ GL_DEPTH24_STENCIL8, mirror_w, mirror_h, mirror_samples };
        FrameBuffer mirror_fbo;
        mirror_fbo.attachTexture(GL_COLOR_ATTACHMENT0, mirror_ms_texture);
        mirror_fbo.attachRBO(GL_DEPTH_STENCIL_ATTACHMENT, mirror_rbo);
        if (!mirror_fbo.isCompleted()) {
            std::cerr << "Mirror FBO is not completed" << std::endl;
            abort();
        }

        RenderObject mirror;
        mirror.render_shader_ = ShaderProgram {
            resourceDir + "shaders/kernel_1.vert",
            resourceDir + "shaders/kernel_1.frag"
        };
        float identity_kernel[9] {
            0, 0, 0,
            0, 1, 0,
            0, 0, 0
        };
        float edge_kernel[9] {
            1, 1, 1,
            1, -8, 1,
            1, 1, 1
        };
        mirror.render_shader_.setInt("tex.samples", mirror_samples);
        mirror.render_shader_.setInt("tex.tex_MS", 0);
        mirror.render_shader_.setFLoatArr("kernel", identity_kernel, 9);
        mirror.render_state_.depth_test_ = false;
        mirror.mesh_ = createQuadMesh(
            InstanceBuffer::InstanceData{
                .translation_ = glm::vec3{ 0, 0.85, 0 },
                .scale_ = glm::vec3{ 0.3, 0.15, 1.0 }
            }
        );
        
        auto render_shadow_casters = [&](const glm::mat4 &light_space) {
            auto draw_model_shadow = [&](Model &m) {
                for (auto &obj : m.objects_) {
                    if (obj.shadow_shader_.id() == 0) {
                        continue;
                    }
                    obj.shadow_shader_.setMat4("light_space_transform", light_space);
                    obj.renderShadow();
                }
            };
            for (auto &m : objects) {
                draw_model_shadow(m);
            }
            for (auto &m : outline_objects) {
                draw_model_shadow(m);
            }
        };

        auto render_scene = [&](Camera &cam){
            LightData &lights = win_data->lights;
            const float explode_magnitude = std::abs(static_cast<float>(sin(glfwGetTime())));
            explode_shader.setFloat("explode_magnitude", explode_magnitude);
            explode_shadow_shader.setFloat("explode_magnitude", explode_magnitude);

            glViewport(0, 0, shadow_map_size, shadow_map_size);
            glEnable(GL_DEPTH_TEST);
            glCullFace(GL_FRONT);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);

            for (int i = 0; i < lights.counts.x; ++i) {
                directional_shadow_fbos[i].bind();
                glClear(GL_DEPTH_BUFFER_BIT);
                render_shadow_casters(lights.directional[i].light_space_transform_);
            }

            for (int i = 0; i < lights.counts.z; ++i) {
                spot_shadow_fbos[i].bind();
                glClear(GL_DEPTH_BUFFER_BIT);
                render_shadow_casters(lights.spot[i].light_space_transform_);
            }

            FrameBuffer::unbind();
            glCullFace(GL_BACK);
            RenderObject::invalidateCachedState();

            bindShadowMapSamplers(
                general_shader, lights,
                directional_shadow_maps, spot_shadow_maps, dummy_shadow_map, 16
            );

            glViewport(0, 0, win_data->win_width, win_data->win_height);
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (auto &m : objects) {
                m.render();
            }
            for (auto &m : outline_objects) {
                m.render();
            }

            // draw skybox
            skybox.material_.diffuse_textures_[0].bind(0);
            skybox.render_shader_.setMat4(
                "no_translate_view", glm::mat4(glm::mat3(cam.viewMatrix()))
            );
            skybox.render();

            // draw transparent objects
            auto sorted = sortObjectsByDistance(cam, transparent_objects);
            for (auto &obj : sorted) {
                obj.first->render();
            }
        };

        win_data->last_time = steady_clock::now();
        for (unsigned frame = 0; !glfwWindowShouldClose(window.get()); ++frame) {
            if (mirror_ms_texture.width() != win_data->win_width
                || mirror_ms_texture.height() != win_data->win_height / 2) {
                const int w = win_data->win_width;
                const int h = win_data->win_height / 2;
                mirror_ms_texture.reallocate(w, h, GL_RGB, mirror_samples);
                mirror_rbo.reallocate(GL_DEPTH24_STENCIL8, w, h, mirror_samples);
            }

            Camera mirror_camera{ win_data->camera };
            mirror_camera.yaw(-180);
            mirror_camera.pitch(-2 * mirror_camera.pitch());
            CameraData mirror_cam_data{ win_data->cam_data };
            mirror_cam_data.view = mirror_camera.viewMatrix();
            mirror_cam_data.projection = glm::perspective(
                glm::radians(win_data->fov),
                static_cast<float>(mirror_ms_texture.width())
                    / static_cast<float>(mirror_ms_texture.height()),
                0.1f,
                100000.0f
            );
            mirror_fbo.bind();
            glViewport(0, 0, mirror_ms_texture.width(), mirror_ms_texture.height());
            win_data->cam_data_UBO.setSubData(0, sizeof(CameraData), &mirror_cam_data);
            render_scene(mirror_camera);
            mirror_fbo.unbind();

            glViewport(0, 0, win_data->win_width, win_data->win_height);
            win_data->cam_data_UBO.setSubData(0, sizeof(CameraData), &win_data->cam_data);
            render_scene(win_data->camera);

            mirror_ms_texture.bind(0);
            mirror.render();

            auto now = steady_clock::now();
            win_data->delta_time = now - win_data->last_time;
            win_data->last_time = now;
            glfwSwapBuffers(window.get());
            glfwPollEvents();
            processInput(window.get());
        }
        // 先释放窗口与 GL 资源，再 glfwTerminate
        window.reset();
        glfwTerminate();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }
}
