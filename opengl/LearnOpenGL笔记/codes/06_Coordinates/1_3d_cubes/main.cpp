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

    auto last_time = steady_clock::now();
    int expect_fps = 60;
    auto frame_duration = milliseconds(1000 / expect_fps);
    for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
        processInput(window);
        glEnable(GL_DEPTH_TEST);
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
            0.1f,
            100.0f
        );
        shaderProg.setMat4("projTrans", projTrans);

        for (int i = 0; i < sizeof(cubePositions) / sizeof(cubePositions[0]); ++i) {
            glm::mat4 modelTrans{ 1.0 };
            float angle = 20.0f * i;
            angle += (360.0f / 10.0f) * (float)glfwGetTime();
            modelTrans = glm::translate(modelTrans, cubePositions[i]);
            modelTrans = glm::rotate(modelTrans, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            shaderProg.setMat4("modelTrans", modelTrans);
            rectMesh.draw();
        }

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