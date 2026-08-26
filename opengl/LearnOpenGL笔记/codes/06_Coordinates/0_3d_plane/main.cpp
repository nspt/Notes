#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include "ShaderProgram.h"
#include "Mesh.h"
#include "Texture2D.h"
#include "glm/detail/func_trigonometric.hpp"

static constexpr int win_width = 800;
static constexpr int win_height = 600;

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
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

Mesh createCubeMesh()
{
    std::vector<Vertex> vertices = {
        {
            .position = { -0.5f, -0.5f, 0.0f },
            .texCoord = { 0.0f, 0.0f }
        }, // 左下
        {
            .position = { 0.5f, -0.5f, 0.0f },
            .texCoord = { 1.0f, 0.0f }
        }, // 右下
        {
            .position = { 0.5f, 0.5f, 0.0f },
            .texCoord = { 1.0f, 1.0f }
        }, // 右上
        {
            .position = { -0.5f, 0.5f, 0.0f },
            .texCoord = { 0.0f, 1.0f }
        }, // 左上
    };
    
    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    return Mesh{ vertices, indices };
}

int main(int argc, char* argv[])
try {
    using namespace std::chrono;
    using namespace std::chrono_literals;
    
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <resources dir>" << std::endl;
        return -1;
    }
    std::string resourceDir{ argv[1] };

    auto window = initContextAndWindow();

    ShaderProgram shaderProg {
        resourceDir + "shaders/06_coordinates/06coordinates.vert.glsl",
        resourceDir + "shaders/06_coordinates/06coordinates.frag.glsl"
    };
    shaderProg.setInt("sampler0", 0);
    shaderProg.setInt("sampler1", 1);

    Texture2D brickTexture{ resourceDir + "textures/brickwall.jpg", 0 };
    Texture2D faceTexture{ resourceDir + "textures/awesomeface.png", 1 };

    auto rectMesh = createCubeMesh();

    auto last_time = steady_clock::now();
    int expect_fps = 60;
    auto frame_duration = milliseconds(1000 / expect_fps);
    for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProg.use();
        brickTexture.bind(0);
        faceTexture.bind(1);

        glm::mat4 viewTrans{ 1.0 };
        viewTrans = glm::translate(viewTrans, glm::vec3{ 0.0f, 0.0f, -3.0f });
        shaderProg.setMat4("viewTrans", viewTrans);

        glm::mat4 projTrans = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(win_width) / static_cast<float>(win_height),
            0.0f,
            100.0f
        );
        shaderProg.setMat4("projTrans", projTrans);

        glm::mat4 modelTrans{ 1.0 };
        modelTrans = glm::rotate(modelTrans, glm::radians(-55.0f), glm::vec3{1.0f, 0.0f, 0.0f});
        shaderProg.setMat4("modelTrans", modelTrans);
        
        rectMesh.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto elapsed = duration_cast<milliseconds>(steady_clock::now() - last_time);
        if (elapsed < frame_duration)
            std::this_thread::sleep_for(frame_duration - elapsed);
        last_time = steady_clock::now();
    }
    glfwTerminate();
    return 0;
} catch (const std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    glfwTerminate();
    return -1;
}