#include "ShaderProgram.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>

namespace {

std::string readFile(const std::string &path)
{
    std::ifstream file{ path };
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

unsigned int compileShader(const std::string &src, unsigned int type)
{
    auto shader = glCreateShader(type);
    const char* srcPtr = src.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(shader);
        throw std::runtime_error("Compile shader failed: " + std::string(infoLog));
    }
    return shader;
}

} // namespace

ShaderProgram::ShaderProgram(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
{
    auto vSrc = readFile(vertexShaderPath);
    auto fSrc = readFile(fragmentShaderPath);

    unsigned int vShader{ 0 }, fShader{ 0 };
    try {
        vShader = compileShader(vSrc, GL_VERTEX_SHADER);
        fShader = compileShader(fSrc, GL_FRAGMENT_SHADER);
    } catch (const std::runtime_error &e) {
        if (vShader) glDeleteShader(vShader);
        if (fShader) glDeleteShader(fShader);
        throw;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, vShader);
    glAttachShader(m_id, fShader);
    glLinkProgram(m_id);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    int success;
    glGetProgramiv(m_id, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        char infoLog[512];
        glGetProgramInfoLog(m_id, sizeof(infoLog), nullptr, infoLog);
        glDeleteProgram(m_id);
        throw std::runtime_error("Link program failed: " + std::string(infoLog));
    }
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(m_id);
}

void ShaderProgram::use() const
{
    glUseProgram(m_id);
}

void ShaderProgram::setUniform(const std::string &name, int value) const
{
    auto location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) {
        throw std::runtime_error("Uniform not found: " + name);
    }
    glUniform1i(location, value);
}

void ShaderProgram::setUniform(const std::string &name, float value) const
{
    auto location = glGetUniformLocation(m_id, name.c_str());
    if (location == -1) {
        throw std::runtime_error("Uniform not found: " + name);
    }
    glUniform1f(location, value);
}