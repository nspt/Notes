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
#include "Material.h"
#include "ShaderProgram.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "glm/detail/func_trigonometric.hpp"
#include "Camera.h"
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
        },
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

Light createLight()
{
    Light l;
    l.ambient_ = glm::vec3{ 0.2f, 0.2f, 0.2f };
    l.diffuse_ = glm::vec3{ 0.5f, 0.5f, 0.5f };
    l.specular_ = glm::vec3{ 1.0f, 1.0f, 1.0f };
    l.pos_ = glm::vec3{ 0.0f, 10.0f, 0.0f };
    return l;
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

        auto light = createLight();
        glm::mat4 projection = glm::perspective(
            glm::radians(win_data->fov),
            static_cast<float>(win_width) / static_cast<float>(win_height),
            0.1f,
            100.0f
        );
        auto cube_mesh = createCubeMesh();
        auto cube_material = std::make_shared<Material>();
        cube_material->program_ = std::make_shared<ShaderProgram>(
            resourceDir + "shaders/08_lighting/08light_map.vert",
            resourceDir + "shaders/08_lighting/08light_map.frag"
        );
        cube_material->diffuse_map_ = std::make_shared<Texture2D>(resourceDir + "/textures/container2.png");
        cube_material->specular_map_ = std::make_shared<Texture2D>(resourceDir + "/textures/container2_specular.png");
        cube_material->shininess_ = 32.0f;
        std::vector<RenderObject> cubes;
        glm::vec3 cubePositions[] = {
            glm::vec3( 0.0f,  0.0f,  0.0f),
            glm::vec3( 2.0f,  5.0f, -15.0f),
            glm::vec3(-1.5f, -2.2f, -2.5f),
            glm::vec3(-3.8f, -2.0f, -12.3f),
            glm::vec3( 2.4f, -0.4f, -3.5f),
            glm::vec3(-1.7f,  3.0f, -7.5f),
            glm::vec3( 1.3f, -2.0f, -2.5f),
            glm::vec3( 1.5f,  2.0f, -2.5f),
            glm::vec3( 1.5f,  0.2f, -1.5f),
            glm::vec3(-1.3f,  1.0f, -1.5f)
        };
        for (auto &pos : cubePositions) {
            cubes.push_back(RenderObject{});
            auto &cube{ cubes.back() };
            cube.mesh_ = cube_mesh;
            cube.material_ = cube_material;
            cube.transform_ = glm::translate(cube.transform_, pos);
        }

        auto light_cube_material = std::make_shared<Material>();
        light_cube_material = std::make_shared<Material>();
        light_cube_material->program_ = std::make_shared<ShaderProgram>(
            resourceDir + "shaders/08_lighting/08light_src.vert.glsl",
            resourceDir + "shaders/08_lighting/08light_src.frag.glsl"
        );
        RenderObject light_cube;
        light_cube.mesh_ = cube_mesh;
        light_cube.material_ = light_cube_material;
    
        win_data->last_time = steady_clock::now();
        for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 lightRotateMat{1.0f};
            lightRotateMat = glm::rotate(
                lightRotateMat,
                glm::radians(win_data->delta_time.count() * 360.0f / 10.0f),
                glm::vec3{-1.0f, 0.0f, 1.0f}
            );
            light.pos_ = glm::vec3(lightRotateMat * glm::vec4{ light.pos_, 1.0f });
            auto projection = glm::perspective(
                glm::radians(win_data->fov),
                static_cast<float>(win_width) / static_cast<float>(win_height),
                0.1f,
                100.0f
            );

            auto program = cubes.front().material_->program_;
            program->use();
            program->setMat4("projection", projection);
            program->setMat4("view", win_data->camera.viewMatrix());
            program->setVec3("light.ambient", light.ambient_);
            program->setVec3("light.diffuse", light.diffuse_);
            program->setVec3("light.specular", light.specular_);
            program->setVec3("light.view_pos", glm::vec3(win_data->camera.viewMatrix() * glm::vec4{ light.pos_, 1.0f }));
            for (auto &cube : cubes) {
                program->setMat4("model", cube.transform_);
                cube.material_->apply();
                cube.mesh_->draw();
            }

            program = light_cube.material_->program_;
            program->use();
            program->setMat4("projection", projection);
            program->setMat4("view", win_data->camera.viewMatrix());
            program->setVec3("light_color", light.diffuse_);
            program->setMat4("model", glm::translate(glm::mat4{ 1.0f }, light.pos_));
            light_cube.mesh_->draw();
    
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