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

static constexpr int def_win_width = 800;
static constexpr int def_win_height = 600;

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
    float move_speed{ 5.0f };
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
    GLFWwindow* window = glfwCreateWindow(def_win_width, def_win_height, "LearnOpenGL", NULL, NULL);
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
    data.directional[0].ambient_ = glm::vec4{ 0.2 };
    data.directional[0].diffuse_ = glm::vec4{ 0.6 };
    data.directional[0].specular_ = glm::vec4{ 0.9 };

    // 2. point
    data.counts.y = 1;
    data.point[0].pos_ = glm::vec4{ 5, 0, 0, 1 };
    data.point[0].ambient_ = glm::vec4{ 0.2 };
    data.point[0].diffuse_ = glm::vec4{ 0.5 };
    data.point[0].specular_ = glm::vec4{ 1.0 };
    data.point[0].attenuation = glm::vec4{ 1.0, 0.027, 0.0028, 0.0 };

    // 3. spot
    data.counts.z = 2;
    data.spot[0].pos_ = glm::vec4{ 0, 10, 10, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[0].pos_));
        data.spot[0].direction_inner_ = glm::vec4{ dir, 0.99 };
    }
    data.spot[0].ambient_ = glm::vec4{ 0.2 };
    data.spot[0].diffuse_ = glm::vec4{ 0.8 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    data.spot[0].attenuation_outter_ = glm::vec4{ 1.0, 0.027, 0.0028, 0.95 };

    data.spot[1].pos_ = glm::vec4{ 10, 10, 0, 1 };
    {
        auto dir = glm::normalize(glm::vec3{ 0 } - glm::vec3(data.spot[1].pos_));
        data.spot[1].direction_inner_ = glm::vec4{ dir, 0.99 };
    }
    data.spot[1].ambient_ = glm::vec4{ 0.2 };
    data.spot[1].diffuse_ = glm::vec4{ 0.8 };
    data.spot[1].specular_ = glm::vec4{ 1.0 };
    data.spot[1].attenuation_outter_ = glm::vec4{ 1.0, 0.027, 0.0028, 0.95 };
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

Mesh createGrassMesh(InstanceBuffer ibo = InstanceBuffer{ InstanceBuffer::InstanceData{} })
{
    const float x = 0.5f, y = 0.5f, z = 0.5f;
    static std::vector<Vertex> vertices = {
        { { -x, -y, z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { x, -y, z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { x, y, z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
        { { -x, y, z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
    };
    static std::vector<std::uint32_t> indices = {
        0, 1, 2,
        0, 2, 3,
    };
    static VertexBuffer vbo{ vertices };
    static IndexBuffer ebo{ indices };

    return Mesh{ vbo, ebo, ibo };
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

std::vector<RenderObject> createGrasses(const std::string &resourceDir, ShaderProgram shader)
{
    Material material;
    material.program_ = shader;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/grass.png"));
    material.diffuse_textures_[0].setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    std::vector<InstanceBuffer::InstanceData> grasses {
        { .traslation_ = { 7.0f,  -1.0f,  -4.0f } },
        { .traslation_ = { 0.0f,  -1.0f,  -4.0f } },
        { .traslation_ = { -7.0f, -1.0f, -3.0f } },
        { .traslation_ = { -7.0f, -1.0f, 0.0f } },
        { .traslation_ = { 7.0f,  -1.0f,  0.0f } },
        { .traslation_ = { -7.0f, -1.0f, 3.0f } },
        { .traslation_ = { 0.0f, -1.0f, 3.0f } },
        { .traslation_ = { 7.0f,  -1.0f,  4.0f } },
    };
    return {
        RenderObject{
            material, createQuadMesh(InstanceBuffer{ std::move(grasses) })
        }
    };
}

std::vector<RenderObject> createWindows(const std::string &resourceDir, ShaderProgram shader)
{
    Material material;
    material.program_ = shader;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/window.png"));
    material.diffuse_textures_[0].setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    std::vector<InstanceBuffer::InstanceData> windows {
        { .traslation_ = { 7.0f,  -1.0f,  -4.0f } },
        { .traslation_ = { 0.0f,  -1.0f,  -4.0f } },
        { .traslation_ = { -7.0f, -1.0f, -3.0f } },
        { .traslation_ = { -7.0f, -1.0f, 0.0f } },
        { .traslation_ = { 7.0f,  -1.0f,  0.0f } },
        { .traslation_ = { -7.0f, -1.0f, 3.0f } },
        { .traslation_ = { 0.0f, -1.0f, 3.0f } },
        { .traslation_ = { 7.0f,  -1.0f,  4.0f } },
    };
    return {
        RenderObject{
            material, createQuadMesh(windows)
        }
    };
}

RenderObject createPlatform(const std::string &resourceDir, ShaderProgram shader)
{
    Material material;
    material.program_ = shader;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/container2.png"));
    material.specular_textures_.push_back(Texture2D(resourceDir + "/textures/container2_specular.png"));
    material.shininess_ = 128.0f;

    InstanceBuffer::InstanceData platform;
    platform.traslation_ = glm::vec3{ 0.0f, -50.0f, 0.0f };
    platform.scale_ = glm::vec3{ 100.0f };

    return RenderObject{ material, createCubeMesh(platform) };
}

std::vector<RenderObject> createCubes(const std::string &resourceDir, ShaderProgram shader)
{
    Material material;
    material.program_ = shader;
    material.diffuse_textures_.push_back(Texture2D(resourceDir + "/textures/container2.png"));
    material.specular_textures_.push_back(Texture2D(resourceDir + "/textures/container2_specular.png"));
    material.shininess_ = 128.0f;

    std::vector<InstanceBuffer::InstanceData> cubes {
        { .traslation_ = { 0.0f,  1.0f,  0.0f }, .scale_ = { 2, 2, 2 } },
        { .traslation_ = { 10.0f,  0.5f,  0.0f } },
        { .traslation_ = { 10.0f,  1.7f,  0.0f } }
    };

    std::vector<RenderObject> objects;
    for (auto &cube : cubes) {
        objects.emplace_back(material, createCubeMesh(cube));
    }
    return objects;
}

void render(RenderObject &obj, GLsizei count = 1, ShaderProgram *program = nullptr)
{
    if (!program) {
        program = &obj.material_.program_;
    }
    if (program) {
        program->use();
        obj.material_.apply(*program);
    }
    obj.mesh_.draw(count);
}

void render(Model &m, GLsizei count = 1, ShaderProgram *program = nullptr)
{
    if (!program) {
        program = &m.program_;
    }
    for (auto &obj : m.objects_) {
        render(obj, count, program);
    }
}

void renderOutline(RenderObject &obj, const glm::vec3 &color, GLsizei count = 1, ShaderProgram *program = nullptr)
{
    if (!program) {
        program = &obj.material_.program_;
    }
    glStencilFunc(GL_NOTEQUAL, 1, 0xff);
    glDepthFunc(GL_ALWAYS);
    Material outline;
    outline.pure_color_ = true;
    outline.color_ = color;
    if (program) {
        program->use();
        outline.apply(*program);
    }
    obj.mesh_.draw(count);
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glDepthFunc(GL_LESS);
}

void renderOutline(Model &m, const glm::vec3 &color, GLsizei count = 1, ShaderProgram *program = nullptr)
{
    glStencilFunc(GL_NOTEQUAL, 1, 0xff);
    glDepthFunc(GL_ALWAYS);
    Material outline;
    outline.pure_color_ = true;
    outline.color_ = color;
    for (auto &obj : m.objects_) {
        if (program) {
            program->use();
            outline.apply(*program);
        } else if (obj.material_.program_.id() != 0) {
            obj.material_.program_.use();
            outline.apply(obj.material_.program_);
        } else if (m.program_.id() != 0) {
            m.program_.use();
            outline.apply(m.program_);
        }
        obj.mesh_.draw(count);
    }
    glStencilFunc(GL_ALWAYS, 0, 0xff);
    glDepthFunc(GL_LESS);
}

std::vector<std::pair<RenderObject*, float>> sortObjectsByDistance(WinData &win_data, const std::vector<RenderObject> &objs)
{
    auto cam_pos = win_data.camera.pos();
    std::vector<std::pair<RenderObject*, float>> sorted;
    sorted.reserve(objs.size());
    for (auto &obj : objs) {
        sorted.push_back({
            const_cast<RenderObject*>(&obj),
            glm::distance(cam_pos, obj.mesh_.instanceBuffer().data().front().traslation_)
        });
    }
    std::sort(sorted.begin(), sorted.end(), [cam_pos](auto &a, auto &b) {
        return a.second > b.second;
    });
    return sorted;
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

        ShaderProgram general_shader{
            resourceDir + "shaders/general_0.vert",
            resourceDir + "shaders/general_0.frag"
        };
        general_shader.setUniformBlockBinding("LightData", 0);
        general_shader.setUniformBlockBinding("CamData", 1);

        ShaderProgram visual_normal_shader{
            resourceDir + "shaders/visual_normal_0.vert",
            resourceDir + "shaders/visual_normal_0.frag",
            resourceDir + "shaders/visual_normal_0.geom"
        };
        visual_normal_shader.setUniformBlockBinding("CamData", 1);
        visual_normal_shader.setVec3("normal_color", glm::vec3{ 0, 1, 0 });

        auto cubes = createCubes(resourceDir, general_shader);
        auto platform = createPlatform(resourceDir, general_shader);

        std::vector<Model> flashlights;
        {
            Model flashlight{ resourceDir + "/model/flash_light", "Flashlight.obj", general_shader, false };
            for (int i = 0; i < win_data->lights.counts.z; ++i) {
                auto &spot{ win_data->lights.spot[i] };
                InstanceBuffer::InstanceData instance;
                instance.traslation_ = spot.pos_;
                
                glm::vec3 direction{ spot.direction_inner_ };
                float yaw = atan2(direction.x, direction.z);
                float horizontal = std::hypot(direction.x, direction.z);
                float pitch = atan2(-direction.y, horizontal);
                instance.rotation_ = glm::angleAxis(yaw, glm::vec3{ 0, 1, 0 })
                    * glm::angleAxis(pitch, glm::vec3{ 1, 0, 0 });

                instance.scale_ = glm::vec3{ 0.3f };
                flashlights.push_back(flashlight);
                flashlights.back().setInstances(InstanceBuffer{ instance });
            }
        }

        Model backpack{ resourceDir + "/model/backpack", "backpack.obj", general_shader };
        backpack.setInstances(InstanceBuffer::InstanceData{ .traslation_ = { -5, 2, 0 } });

        Model visual_normal_backpack{ backpack };
        visual_normal_backpack.setInstances(InstanceBuffer::InstanceData{ .traslation_ = { -5, 2, -5 } });

        Model explode_backpack{ backpack };
        explode_backpack.setInstances(InstanceBuffer::InstanceData{ .traslation_ = { 0, 4, -5 } });
        explode_backpack.program_ = ShaderProgram{
            resourceDir + "shaders/explode_0.vert",
            resourceDir + "shaders/explode_0.frag",
            resourceDir + "shaders/explode_0.geom"
        };
        explode_backpack.program_.setUniformBlockBinding("LightData", 0);
        explode_backpack.program_.setUniformBlockBinding("CamData", 1);

        glm::vec3 planet_pos{ 10, 30, 0 };
        Model planet{ resourceDir + "/model/planet", "planet.obj", general_shader };
        planet.setInstances(InstanceBuffer::InstanceData{ .traslation_ = planet_pos });

        Model asteroid { resourceDir + "/model/rock", "rock.obj", general_shader };
        std::vector<InstanceBuffer::InstanceData> rocks;
        {
            glm::vec3 rock_offset{ 25, 0, 0 };
            glm::vec3 planet_axis = glm::normalize(glm::vec3{ 0, 1, 0.5 });
            std::vector<glm::mat4> rock_transforms;
            std::mt19937 rand_gen{ std::random_device{}() };
            std::normal_distribution<float> normal_dist{ 0, 1 };
            for (int i = 0; i < 10000; ++i) {
                glm::vec3 rand_offset{ normal_dist(rand_gen), normal_dist(rand_gen), normal_dist(rand_gen) };
                float orbit_angle = 360.0f / 1000.0f * i;

                glm::quat orbit_rotation = glm::angleAxis(glm::radians(orbit_angle),
                    glm::normalize(planet_axis));
                glm::quat local_rotation = glm::angleAxis(normal_dist(rand_gen),
                    glm::normalize(rand_offset));
                InstanceBuffer::InstanceData instance;
                instance.traslation_ = planet_pos +
                    orbit_rotation * (rock_offset + rand_offset);
                instance.rotation_ = orbit_rotation * local_rotation;
                instance.scale_ = glm::vec3{ 0.1 };
                rocks.push_back(instance);
            }
        }
        asteroid.setInstances(rocks);

        win_data->last_time = steady_clock::now();
        for (unsigned frame = 0; !glfwWindowShouldClose(window.get()); ++frame) {
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (auto &cube : cubes) {
                render(cube);
            }
            render(platform);
            for (auto &m : flashlights) {
                render(m);
            }
            render(backpack);
            render(visual_normal_backpack);
            render(visual_normal_backpack, 1, &visual_normal_shader);
            explode_backpack.program_.setFloat("explode_magnitude", std::abs(sin(glfwGetTime())));
            render(explode_backpack);
            render(planet);
            render(asteroid, rocks.size());
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