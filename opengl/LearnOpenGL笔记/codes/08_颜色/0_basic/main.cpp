#include <chrono>
#include <cstddef>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "glm/detail/func_geometric.hpp"
#include "glm/detail/func_trigonometric.hpp"
#include "Camera.h"
#include "glm/detail/type_vec.hpp"

static constexpr int win_width = 800;
static constexpr int win_height = 600;

struct WinData
{
    Camera camera{ glm::vec3{ 0.0f, 0.0f, 10.0f } };
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
    front.y = 0;
    front = glm::normalize(front);
    auto right = camera.right();
    right.y = 0;
    right = glm::normalize(right);
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

GLFWwindow* initContextAndWindow()
{
    initGLFW(3, 3);
    auto window = createWindow();
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error{ "Failed to init GLAD" };
    printOpenGLInfo();
    return window;
}

Mesh createMesh()
{
    float x = 0.5f, y = 0.5f, z = 0.5f;
    std::vector<Vertex> vertices = {
        // front
        { { -x, -y, z }, { 0.0f, 0.0f }},
        { { x, -y, z }, { 1.0f, 0.0f }},
        { { x, y, z }, { 1.0f, 1.0f }},
        { { -x, y, z }, { 0.0f, 1.0f }},
        // back
        { { x, -y, -z }, { 0.0f, 0.0f }},
        { { -x, -y, -z }, { 1.0f, 0.0f }},
        { { -x, y, -z }, { 1.0f, 1.0f }},
        { { x, y, -z }, { 0.0f, 1.0f }},
        // left
        { { -x, -y, -z }, { 0.0f, 0.0f }},
        { { -x, -y, z }, { 1.0f, 0.0f }},
        { { -x, y, z }, { 1.0f, 1.0f }},
        { { -x, y, -z }, { 0.0f, 1.0f }},
        // right
        { { x, -y, z }, { 0.0f, 0.0f }},
        { { x, -y, -z }, { 1.0f, 0.0f }},
        { { x, y, -z }, { 1.0f, 1.0f }},
        { { x, y, z }, { 0.0f, 1.0f }},
        // top
        { { -x, y, z }, { 0.0f, 0.0f }},
        { { x, y, z }, { 1.0f, 0.0f }},
        { { x, y, -z }, { 1.0f, 1.0f }},
        { { -x, y, -z }, { 0.0f, 1.0f }},
        // bottom
        { { x, -y, z }, { 0.0f, 0.0f }},
        { { -x, -y, z }, { 1.0f, 0.0f }},
        { { -x, -y, -z }, { 1.0f, 1.0f }},
        { { x, -y, -z }, { 0.0f, 1.0f }},
    };

    std::vector<VertexAttrib> attributes {
        VertexAttrib {
            0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, position))
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

    return Mesh{ vertices, attributes, indices };
}

int main(int argc, char* argv[])
{
    try {
        using namespace std::chrono;
        using namespace std::chrono_literals;
        
        if (argc != 2) {
            std::cout << "Usage: " << argv[0] << " <resources dir>" << std::endl;
            return -1;
        }
        std::string resourceDir{ argv[1] };
    
        auto window = initContextAndWindow();
        auto win_data = std::make_unique<WinData>();
        glfwSetWindowUserPointer(window, win_data.get());
    
        glm::vec3 lightColor{ 1.0f, 1.0f, 1.0f };
        ShaderProgram objShaderProg {
            resourceDir + "shaders/08color_basic.vert.glsl",
            resourceDir + "shaders/08color_basic.frag.glsl"
        };
        objShaderProg.setVec3("lightColor", lightColor);
        objShaderProg.setVec3("objectColor", glm::vec3{ 1.0f, 0.5f, 0.31f });
        glm::mat4 projTrans = glm::perspective(
            glm::radians(win_data->fov),
            static_cast<float>(win_width) / static_cast<float>(win_height),
            0.1f,
            100.0f
        );
        objShaderProg.setMat4("projTrans", projTrans);

        ShaderProgram lightShaderProg {
            resourceDir + "shaders/08color_basic.vert.glsl",
            resourceDir + "shaders/08color_basic_light.frag.glsl"
        };
        lightShaderProg.setVec3("lightColor", lightColor);
        lightShaderProg.setMat4("projTrans", projTrans);
    
        Texture2D brickTexture{ resourceDir + "textures/brickwall.jpg", 0 };
        Texture2D faceTexture{ resourceDir + "textures/awesomeface.png", 1 };
    
        auto rectMesh = createMesh();
    
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
    
        win_data->last_time = steady_clock::now();
        for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
            glEnable(GL_DEPTH_TEST);
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
            objShaderProg.use();
            brickTexture.bind(0);
            faceTexture.bind(1);
    
            objShaderProg.setMat4("viewTrans", win_data->camera.viewMatrix());

            glm::mat4 modelTrans{ 1.0 };
            modelTrans = glm::translate(modelTrans, glm::vec3{ 0.0f, 0.0f, 2.0f });
            objShaderProg.setMat4("modelTrans", modelTrans);
            rectMesh.draw();

            lightShaderProg.use();
            lightShaderProg.setMat4("viewTrans", win_data->camera.viewMatrix());

            glm::mat4 lightModelTrans{ 1.0 };
            lightModelTrans = glm::translate(lightModelTrans, glm::vec3{ 0.0f, 2.0f, -2.0f });
            lightShaderProg.setMat4("modelTrans", lightModelTrans);
            rectMesh.draw();
    
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