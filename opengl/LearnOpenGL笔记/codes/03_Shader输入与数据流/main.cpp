#include <chrono>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <tuple>
#include "ShaderProgram.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

std::tuple<unsigned, unsigned, unsigned> createMesh(float* vertices, unsigned int* indices, size_t verticesSize, size_t indicesSize)
{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesSize, vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return { VBO, EBO, VAO };
}

std::tuple<unsigned, unsigned, unsigned> createLeftTriangle()
{
    float vertices[] = {
        -0.6f, -0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -0.1f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.1f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = {
        0, 1, 2
    };
    
    return createMesh(vertices, indices, sizeof(vertices), sizeof(indices));
}

std::tuple<unsigned, unsigned, unsigned> createRightTriangle()
{
    float vertices[] = {
        0.6f, -0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.1f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.1f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f
    };
    unsigned int indices[] = {
        0, 1, 2
    };
    
    return createMesh(vertices, indices, sizeof(vertices), sizeof(indices));
}

void drawTriangle(unsigned fillVAO, unsigned lineVAO)
{
    glBindVertexArray(fillVAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
    
    glBindVertexArray(lineVAO);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
}

bool checkShaderCompile(unsigned int shader, const char* type)
{
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::" << type << "::COMPILE\n" << infoLog << std::endl;
    }
    return success;
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwMakeContextCurrent(window);
    return window;
}

void initGLAD()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        exit(-1);
    }
}

int main(int argc, char* argv[])
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    try {
        if (argc != 3) {
            std::cout << "Usage: " << argv[0] << " <vertex shader> <fragment shader>" << std::endl;
            return -1;
        }
    
        initGLFW(3, 3);
        auto window = createWindow();
        initGLAD();
        printOpenGLInfo();
    
        auto [lVBO, lEBO, lVAO] = createLeftTriangle();
        auto [rVBO, rEBO, rVAO] = createRightTriangle();
        
        ShaderProgram shaderProgram(argv[1], argv[2]);

        auto last_time = steady_clock::now();
        int expect_fps = 30;
        auto frame_duration = milliseconds(1000 / expect_fps);
        for (unsigned frame = 0; !glfwWindowShouldClose(window); ++frame) {
            processInput(window);
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
    
            shaderProgram.use();
            unsigned phase = frame % (expect_fps * 2);
            if (phase >= expect_fps) {
                shaderProgram.setUniform("extra_red_value", 0.5f);
                drawTriangle(rVAO, lVAO);
            } else {
                shaderProgram.setUniform("extra_red_value", 0.0f);
                drawTriangle(lVAO, rVAO);
            }
    
            glfwSwapBuffers(window);
            glfwPollEvents();
    
            auto elapsed = duration_cast<milliseconds>(steady_clock::now() - last_time);
            if (elapsed < frame_duration)
                std::this_thread::sleep_for(frame_duration - elapsed);
            last_time = steady_clock::now();
        }
        glDeleteVertexArrays(1, &lVAO);
        glDeleteBuffers(1, &lVBO);
        glDeleteBuffers(1, &lEBO);
    
        glDeleteVertexArrays(1, &rVAO);
        glDeleteBuffers(1, &rVBO);
        glDeleteBuffers(1, &rEBO);
    
        glfwTerminate();
        return 0;
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
