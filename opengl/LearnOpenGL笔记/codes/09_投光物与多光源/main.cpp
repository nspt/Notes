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
#include "glm/detail/func_trigonometric.hpp"
#include "Camera.h"
#include "glm/detail/type_mat.hpp"
#include "glm/detail/type_vec.hpp"
#include "RenderObject.h"
#include "Light.h"

static constexpr int win_width = 800;
static constexpr int win_height = 600;

struct WinData
{
    Camera camera{ Camera::Type::Fly, glm::vec3{ 0.0f, 0.0f, 5.0f } };
    std::chrono::time_point<std::chrono::steady_clock> last_time{ std::chrono::steady_clock::now() };
    std::chrono::duration<float> delta_time{ 0 };
    float fov{ 45.0f };
    bool first_mouse{ true };
    double mouse_x{ 0.0 }, mouse_y{ 0.0 };
    float move_speed{ 1.0f };
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
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
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

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
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
    glEnable(GL_DEPTH_TEST);
    
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
        0, 2, 1,
        0, 2, 3,
        // left
        4, 6, 5,
        4, 6, 7,
        // right
        8, 10, 9,
        8, 10, 11,
        // back
        12, 14, 13,
        12, 14, 15,
        // top
        16, 18, 17,
        16, 18, 19,
        // bottom
        20, 22, 21,
        20, 22, 23,
    };

    return std::make_shared<Mesh>(vertices, attributes, indices);
}

LightData createLightData()
{
    LightData data;

    // 1. directional
    data.counts.x = 1;
    data.directional[0].direction_view_ = glm::vec4{ 0, -1, 0, 0 };
    data.directional[0].ambient_ = glm::vec4{ 0.08 };
    data.directional[0].diffuse_ = glm::vec4{ 0.3 };
    data.directional[0].specular_ = glm::vec4{ 0.3 };

    // 2. point
    data.counts.y = 1;
    data.point[0].pos_view_ = glm::vec4{ 0, 0, 0, 0 };
    data.point[0].ambient_ = glm::vec4{ 0.2 };
    data.point[0].diffuse_ = glm::vec4{ 0.5 };
    data.point[0].specular_ = glm::vec4{ 1.0 };
    data.point[0].attenuation = glm::vec4{ 1.0, 0.027, 0.0028, 0.0 };

    // 3. spot
    data.counts.z = 1;
    data.spot[0].pos_view_ = glm::vec4{ 0, 0, 0, 0 };
    data.spot[0].direction_inner_ = glm::vec4{ 0, 0, 0, 0.99 };
    data.spot[0].ambient_ = glm::vec4{ 0.2 };
    data.spot[0].diffuse_ = glm::vec4{ 0.5 };
    data.spot[0].specular_ = glm::vec4{ 1.0 };
    data.spot[0].attenuation_outter_ = glm::vec4{ 1.0, 0.027, 0.0028, 0.95 };

    return data;
}

void applyLightData(WinData &win_data, LightData &lights, UniformBuffer &lightDataUBO)
{
    lights.point[0].pos_ = glm::vec4{ 5, 0, 0, 1 };
    lights.point[0].pos_view_ = win_data.camera.viewMatrix() * lights.point[0].pos_;

    lights.spot[0].pos_ = glm::vec4{ -1, 0, 0, 1 };
    lights.spot[0].pos_view_ = win_data.camera.viewMatrix() * lights.spot[0].pos_;
    auto inner_cutoff = lights.spot[0].direction_inner_.w;
    lights.spot[0].direction_inner_ = glm::normalize(win_data.camera.viewMatrix() * glm::vec4{ 1, 0, 0, 0 });
    lights.spot[0].direction_inner_.w = inner_cutoff;

    lightDataUBO.setSubData(0, sizeof(LightData), &lights);
}

void renderCubes(WinData &win_data, std::vector<RenderObject> &cubes)
{
    auto projection = glm::perspective(
        glm::radians(win_data.fov),
        static_cast<float>(win_width) / static_cast<float>(win_height),
        0.1f,
        100.0f
    );
    auto program = cubes.front().material_->program_;
    program->use();
    program->setMat4("projection", projection);
    program->setMat4("view", win_data.camera.viewMatrix());
    for (auto &cube : cubes) {
        program->setMat4("model", cube.transform_);
        cube.material_->apply();
        cube.mesh_->draw();
    }
}

void renderLightSrc(WinData &win_data, LightData &lights, RenderObject &obj)
{
    auto projection = glm::perspective(
        glm::radians(win_data.fov),
        static_cast<float>(win_width) / static_cast<float>(win_height),
        0.1f,
        100.0f
    );
    auto program = obj.material_->program_;
    program->use();
    program->setMat4("projection", projection);
    program->setMat4("view", win_data.camera.viewMatrix());

    for (int i = 0; i < lights.counts.y; ++i) {
        auto model = glm::translate(glm::mat4{ 1.0 }, glm::vec3(lights.point[i].pos_));
        model = glm::scale(model, glm::vec3{ 0.2, 0.2, 0.2 });
        program->setVec3("light_color", glm::vec3{ 0.7 });
        program->setMat4("model", model);
        obj.mesh_->draw();
    }
    for (int i = 0; i < lights.counts.z; ++i) {
        auto model = glm::translate(glm::mat4{ 1.0 }, glm::vec3(lights.spot[i].pos_));
        model = glm::scale(model, glm::vec3{ 0.1, 0.1, 0.1 });
        program->setVec3("light_color", glm::vec3{ 1.0 });
        program->setMat4("model", model);
        obj.mesh_->draw();
    }
}

std::vector<RenderObject> createCubes(std::shared_ptr<Mesh> cube_mesh, std::shared_ptr<Material> cube_material)
{
    std::vector<RenderObject> cubes;
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        //glm::vec3( 2.0f,  5.0f, -15.0f),
        //glm::vec3(-1.5f, -2.2f, -2.5f),
        //glm::vec3(-3.8f, -2.0f, -12.3f),
        //glm::vec3( 2.4f, -0.4f, -3.5f),
        //glm::vec3(-1.7f,  3.0f, -7.5f),
        //glm::vec3( 1.3f, -2.0f, -2.5f),
        //glm::vec3( 1.5f,  2.0f, -2.5f),
        //glm::vec3( 1.5f,  0.2f, -1.5f),
        //glm::vec3(-1.3f,  1.0f, -1.5f)
    };
    for (auto &pos : cubePositions) {
        cubes.push_back(RenderObject{});
        auto &cube{ cubes.back() };
        cube.mesh_ = cube_mesh;
        cube.material_ = cube_material;
        cube.transform_ = glm::translate(cube.transform_, pos);
    }
    return cubes;
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

        auto cube_mesh = createCubeMesh();
        auto cube_material = std::make_shared<Material>();
        cube_material->program_ = std::make_shared<ShaderProgram>(
            resourceDir + "shaders/09_multiple_lights/09multiple_lights.vert",
            resourceDir + "shaders/09_multiple_lights/09multiple_lights.frag"
        );
        cube_material->program_->setUniformBlockBinding("LightData", 0);
        cube_material->diffuse_map_ = std::make_shared<Texture2D>(resourceDir + "/textures/container2.png");
        cube_material->specular_map_ = std::make_shared<Texture2D>(resourceDir + "/textures/container2_specular.png");
        cube_material->shininess_ = 128.0f;

        auto light_src_material = std::make_shared<Material>();
        light_src_material->program_ = std::make_shared<ShaderProgram>(
            resourceDir + "shaders/09_multiple_lights/08light_src.vert.glsl",
            resourceDir + "shaders/09_multiple_lights/08light_src.frag.glsl"
        );
        light_src_material->program_->setVec3("light_color", glm::vec3{ 1.0, 1.0, 1.0 });
        
        auto cubes = createCubes(cube_mesh, cube_material);

        RenderObject light_src{};
        light_src.mesh_ = cube_mesh;
        light_src.material_ = light_src_material;
    
        win_data->last_time = steady_clock::now();
        for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            applyLightData(*win_data, lights, lightDataUBO);
            renderCubes(*win_data, cubes);
            renderLightSrc(*win_data, lights, light_src);
    
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