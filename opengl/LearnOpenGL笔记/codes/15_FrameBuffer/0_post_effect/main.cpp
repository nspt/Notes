#include <chrono>
#include <cstddef>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <stdexcept>
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
#include <stb_image.h>

static constexpr int win_width = 800;
static constexpr int win_height = 600;

struct WinData
{
    Camera camera{ Camera::Type::Fly, glm::vec3{ 0.0f, 0.0f, 5.0f } };
    std::chrono::time_point<std::chrono::steady_clock> last_time{ std::chrono::steady_clock::now() };
    std::chrono::duration<float> delta_time{ 0 };
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(win_width) / static_cast<float>(win_height),
        1.0f,
        100000.0f
    );
    float fov{ 45.0f };
    bool first_mouse{ true };
    double mouse_x{ 0.0 }, mouse_y{ 0.0 };
    float move_speed{ 10.0f };
    float rotate_sensitivity{ 0.05f };
    float zoom_sensitivity{ 0.1f };
};

#define MAX_DIRECTIONAL_LIGHTS 2
#define MAX_SPOT_LIGHTS 4
#define MAX_POINT_LIGHTS 8
struct LightData {
    glm::ivec4 counts{ 0 }; // x: directional, y: point, z: spot
    DirectionalLight directional[MAX_DIRECTIONAL_LIGHTS];
    SpotLight spot[MAX_SPOT_LIGHTS];
    PointLight point[MAX_POINT_LIGHTS];
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
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto data = static_cast<WinData*>(glfwGetWindowUserPointer(window));
    data->fov -= (float)yoffset;
    if (data->fov < 1.0f)
        data->fov = 1.0f;
    if (data->fov > 45.0f)
        data->fov = 45.0f; 
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
    GLFWwindow* window = glfwCreateWindow(win_width, win_height, "LearnOpenGL", NULL, NULL);
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

std::shared_ptr<Mesh> createGrassMesh()
{
    float x = 0.5f, y = 0.5f, z = 0.5f;
    std::vector<Vertex> vertices = {
        { { -x, -y, z }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { x, -y, z }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { x, y, z }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
        { { -x, y, z }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
    };

    std::vector<VertexAttrib> attributes {
        VertexAttrib {
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position))
        },
        VertexAttrib {
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal))
        },
        VertexAttrib {
            2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord))
        }
    };
    
    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 2, 3,
    };

    return std::make_shared<Mesh>(vertices, attributes, indices);
}

std::shared_ptr<Mesh> createQuadMesh()
{
    std::vector<Vertex> vertices = {
        { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { 1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }},
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
        { { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
    };

    std::vector<VertexAttrib> attributes {
        VertexAttrib {
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position))
        },
        VertexAttrib {
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal))
        },
        VertexAttrib {
            2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord))
        }
    };
    
    std::vector<unsigned int> indices = {
        0, 1, 2,
        0, 2, 3,
    };

    return std::make_shared<Mesh>(vertices, attributes, indices);
}

std::shared_ptr<Mesh> createCubeMesh()
{
    float x = 0.5f, y = 0.5f, z = 0.5f;
    std::vector<Vertex> vertices = {
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

    std::vector<VertexAttrib> attributes {
        VertexAttrib {
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position))
        },
        VertexAttrib {
            1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, normal))
        },
        VertexAttrib {
            2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, texCoord))
        }
    };
    
    std::vector<unsigned int> indices = {
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

    return std::make_shared<Mesh>(vertices, attributes, indices);
}

LightData createLightData()
{
    LightData data;

    // 1. directional
    data.counts.x = 1;
    data.directional[0].direction_ = glm::vec4{ 0, -1, 0, 0 };
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
    data.counts.z = 1;
    data.spot[0].pos_ = glm::vec4{ 0, 0, 2, 1 };
    data.spot[0].direction_inner_ = glm::vec4{ 0, 0, 0, 0.99 };
    data.spot[0].ambient_ = glm::vec4{ 0.2 };
    data.spot[0].diffuse_ = glm::vec4{ 0.5 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    data.spot[0].attenuation_outter_ = glm::vec4{ 1.0, 0.027, 0.0028, 0.95 };

    return data;
}

void applyLightData(WinData &win_data, LightData &lights, UniformBuffer &lightDataUBO)
{
    auto view = win_data.camera.viewMatrix();

    lights.directional[0].direction_view_ = glm::vec4{ glm::mat3(view) * lights.directional[0].direction_, 0 };

    lights.point[0].pos_view_ = view * lights.point[0].pos_;

    lights.spot[0].pos_view_ = view * lights.spot[0].pos_;
    auto inner_cutoff = lights.spot[0].direction_inner_.w;
    lights.spot[0].direction_inner_ = glm::normalize(view * glm::vec4{ 1, 0, 0, 0 });
    lights.spot[0].direction_inner_.w = inner_cutoff;

    lightDataUBO.setSubData(0, sizeof(LightData), &lights);
}

void render(
    RenderObject &obj,
    const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj,
    std::shared_ptr<ShaderProgram> program)
{
    if (!program) {
        program = obj.material_->program_;
    }
    program->use();
    program->setMat4("projection", proj);
    program->setMat4("view", view);
    program->setMat4("model", obj.transform_ * model);
    obj.material_->apply(*program);
    obj.mesh_->draw();
}

void render(Model &m, const glm::mat4 &view, const glm::mat4 &proj)
{
    for (auto &obj : m.objects_) {
        render(*obj, m.transform_, view, proj, m.program_);
    }
}


void renderOutline(
    RenderObject &obj,
    const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &proj,
    std::shared_ptr<ShaderProgram> program)
{
    if (!program) {
        program = obj.material_->program_;
    }
    program->use();
    program->setMat4("projection", proj);
    program->setMat4("view", view);
    program->setMat4("model", obj.transform_ * model * glm::scale(glm::mat4{ 1.0f }, glm::vec3{ 1.2f, 1.2f, 1.2f }));
    auto pure_color = obj.material_->pure_color_;
    auto color = obj.material_->color_;
    obj.material_->pure_color_ = true;
    obj.material_->color_ = glm::vec3{ 1.0f, 0.0f, 0.0f };
    obj.material_->apply(*program);
    obj.material_->pure_color_ = pure_color;
    obj.material_->color_ = color;
    obj.mesh_->draw();
}

void renderOutline(Model &m, const glm::mat4 &view, const glm::mat4 &proj)
{
    for (auto &obj : m.objects_) {
        renderOutline(*obj, m.transform_, view, proj, m.program_);
    }
}

std::vector<RenderObject> assembleObjects(
    std::shared_ptr<Mesh> mesh,
    std::shared_ptr<Material> material,
    const std::vector<std::pair<glm::vec3, glm::mat4>> &pos_and_trans)
{
    std::vector<RenderObject> objs;
    objs.reserve(pos_and_trans.size());
    
    for (auto &pt : pos_and_trans) {
        objs.push_back(RenderObject{});
        auto &obj{ objs.back() };
        obj.mesh_ = mesh;
        obj.material_ = material;
        obj.transform_ = glm::translate(obj.transform_, pt.first);
        obj.transform_ = obj.transform_ * pt.second;
    }

    return objs;
}

std::vector<std::pair<RenderObject*, float>> sortObjectsByDistance(WinData &win_data, const std::vector<RenderObject> &objs)
{
    auto cam_pos = win_data.camera.pos();
    std::vector<std::pair<RenderObject*, float>> sorted;
    sorted.reserve(objs.size());
    for (auto &obj : objs) {
        sorted.push_back({
            const_cast<RenderObject*>(&obj),
            glm::distance(cam_pos, glm::vec3(obj.transform_[3]))
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
    
        auto w = initContextAndWindow();
        auto window = w.get();
        WinData *win_data = static_cast<WinData*>(glfwGetWindowUserPointer(window));

        auto lights = createLightData();
        UniformBuffer lightDataUBO{ lights };
        lightDataUBO.bindBase(0);

        auto general_shader = std::make_shared<ShaderProgram>(
            resourceDir + "shaders/15_framebuffer/13blending.vert",
            resourceDir + "shaders/15_framebuffer/13blending.frag"
        );
        general_shader->setUniformBlockBinding("LightData", 0);

        auto cube_mesh = createCubeMesh();
        auto cube_material = std::make_shared<Material>();
        cube_material->program_ = general_shader;
        cube_material->diffuse_textures_.push_back(std::make_shared<Texture2D>(resourceDir + "/textures/container2.png"));
        cube_material->specular_textures_.push_back(std::make_shared<Texture2D>(resourceDir + "/textures/container2_specular.png"));
        cube_material->shininess_ = 128.0f;
        glm::mat4 platform_mat{ 1.0f };
        platform_mat = glm::scale(platform_mat, glm::vec3{ 100.0f, 100.0f, 100.0f });
        auto cubes = assembleObjects(cube_mesh, cube_material,
            {
                { glm::vec3{ 10.0f,  0.0f,  0.0f }, glm::mat4{ 1.0f } },
                { glm::vec3{ 10.0f,  1.5f,  0.0f }, glm::mat4{ 1.0f } },
                { glm::vec3{ 0.0f, -52.0f, 0.0f }, platform_mat },
            }
        );

        auto grass_mesh = createGrassMesh();
        auto grass_material = std::make_shared<Material>();
        grass_material->program_ = general_shader;
        grass_material->diffuse_textures_.push_back(std::make_shared<Texture2D>(resourceDir + "/textures/grass.png"));
        grass_material->diffuse_textures_[0]->setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        glm::mat4 grass_mat{ 1.0f };
        grass_mat = glm::scale(grass_mat, glm::vec3{ 2.0f, 2.0f, 2.0f });
        auto grasses = assembleObjects(grass_mesh, grass_material,
            {
                { glm::vec3{ 7.0f,  -1.0f,  -4.0f }, grass_mat },
                { glm::vec3{ 0.0f,  -1.0f,  -4.0f }, grass_mat },
                { glm::vec3{ -7.0f, -1.0f, -3.0f }, grass_mat },
                { glm::vec3{ -7.0f, -1.0f, 0.0f }, grass_mat },
                { glm::vec3{ 7.0f,  -1.0f,  0.0f }, grass_mat },
                { glm::vec3{ -7.0f, -1.0f, 3.0f }, grass_mat },
                { glm::vec3{ 0.0f, -1.0f, 3.0f }, grass_mat },
                { glm::vec3{ 7.0f,  -1.0f,  4.0f }, grass_mat },
            }
        );

        auto window_mesh = grass_mesh;
        auto window_material = std::make_shared<Material>();
        window_material->program_ = general_shader;
        window_material->diffuse_textures_.push_back(std::make_shared<Texture2D>(resourceDir + "/textures/window.png"));
        window_material->diffuse_textures_[0]->setWrapMode(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        glm::mat4 window_mat{ 1.0f };
        window_mat = glm::scale(grass_mat, glm::vec3{ 2.0f, 2.0f, 2.0f });
        auto windows = assembleObjects(window_mesh, window_material,
            {
                { glm::vec3{ 7.0f,  -1.0f,  -3.0f }, window_mat },
                { glm::vec3{ 0.0f,  -1.0f,  -3.0f }, window_mat },
                { glm::vec3{ -7.0f, -1.0f, -2.0f }, window_mat },
                { glm::vec3{ -7.0f, -1.0f, 1.0f }, window_mat },
                { glm::vec3{ 7.0f,  -1.0f,  1.0f }, window_mat },
                { glm::vec3{ -7.0f, -1.0f, 4.0f }, window_mat },
                { glm::vec3{ 0.0f, -1.0f, 4.0f }, window_mat },
                { glm::vec3{ 7.0f,  -1.0f,  5.0f }, window_mat },
            }
        );

        std::vector<RenderObject> semiTranspantObjs;
        semiTranspantObjs.reserve(grasses.size() + windows.size());
        semiTranspantObjs.insert(semiTranspantObjs.end(), grasses.begin(), grasses.end());
        semiTranspantObjs.insert(semiTranspantObjs.end(), windows.begin(), windows.end());

        Model model{ resourceDir + "/model/backpack", "backpack.obj" };
        model.program_ = general_shader;

        RenderObject quad_obj;
        quad_obj.mesh_ = createQuadMesh();
        quad_obj.material_ = std::make_shared<Material>();
        quad_obj.material_->program_ = std::make_shared<ShaderProgram>(
            resourceDir + "shaders/15_framebuffer/15kernel.vert",
            resourceDir + "shaders/15_framebuffer/15kernel.frag"
        );
        float identity_kernel[9] {
            0, 0, 0,
            0, 1, 0,
            0, 1, 0
        };
        float sharpen_kernel[9] {
            -1, -1, -1,
            -1, 9, -1,
            -1, -1, -1
        };
        float blur_kernel[9] {
            1.0 / 16, 2.0 / 16, 1.0 / 16,
            2.0 / 16, 4.0 / 16, 2.0 / 16,
            1.0 / 16, 2.0 / 16, 1.0 / 16
        };
        float edge_kernel[9] {
            1, 1, 1,
            1, -8, 1,
            1, 1, 1
        };
        quad_obj.material_->program_->setFLoatArr("kernel", edge_kernel, 9);
        quad_obj.material_->diffuse_textures_.push_back(
            std::make_shared<Texture2D>(
                win_width, win_height, GL_RGB
            )
        );

        FrameBuffer fbo;
        auto rbo = std::make_shared<RenderBuffer>(GL_DEPTH24_STENCIL8, win_width, win_height);
        fbo.attachTexture(GL_COLOR_ATTACHMENT0, quad_obj.material_->diffuse_textures_[0]);
        fbo.attachRBO(GL_DEPTH_STENCIL_ATTACHMENT, rbo);
        if (!fbo.isCompleted()) {
            std::cerr << "FBO is not completed" << std::endl;
            abort();
        }
    
        win_data->last_time = steady_clock::now();
        for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
            fbo.bind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            auto view = win_data->camera.viewMatrix();

            applyLightData(*win_data, lights, lightDataUBO);

            // draw model
            render(model, view, win_data->projection);

            // draw cubes
            glStencilFunc(GL_ALWAYS, 1, 0xff);
            for (auto iter = cubes.begin(); iter != cubes.end(); ++iter) {
                if (iter + 1 != cubes.end()) {
                    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
                } else { // skip last cube, which is platform
                    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
                }
                render(*iter, glm::mat4{}, view, win_data->projection, nullptr);
            }
            glStencilFunc(GL_ALWAYS, 0, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            // draw outline of cubes
            glStencilFunc(GL_NOTEQUAL, 1, 0xff);
            glDepthFunc(GL_ALWAYS);
            for (auto iter = cubes.begin(); iter != cubes.end(); ++iter) {
                if (iter + 1 == cubes.end()) {
                    continue; // skip last cube, which is platform
                }
                renderOutline(*iter, glm::mat4{}, view, win_data->projection, nullptr);
            }
            glStencilFunc(GL_ALWAYS, 0, 0xff);
            glDepthFunc(GL_LESS);

            // draw grasses
            glEnable(GL_BLEND);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
            glDisable(GL_CULL_FACE);
            auto sorted = sortObjectsByDistance(*win_data, semiTranspantObjs);
            for (auto &obj : sorted) {
                render(*obj.first, glm::mat4{}, view, win_data->projection, nullptr);
            }
            glEnable(GL_CULL_FACE);
            glDisable(GL_BLEND);

            fbo.unbind();
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render(quad_obj, glm::mat4{}, glm::mat4{}, glm::mat4{}, nullptr);

            auto now = steady_clock::now();
            win_data->delta_time = now - win_data->last_time;
            win_data->last_time = now;
            glfwSwapBuffers(window);
            glfwPollEvents();
            processInput(window);
        }
        glfwTerminate();
        return 0;
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }
}