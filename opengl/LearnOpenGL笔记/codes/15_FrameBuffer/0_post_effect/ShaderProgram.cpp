#include "ShaderProgram.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

ShaderProgram::ShaderProgram(const std::filesystem::path& vertexShaderPath,
                             const std::filesystem::path& fragmentShaderPath)
{
    const std::string vertexSource = readTextFile(vertexShaderPath);
    const std::string fragmentSource = readTextFile(fragmentShaderPath);

    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    try {
        id_ = linkProgram(vertexShader, fragmentShader);
    } catch (...) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

ShaderProgram::~ShaderProgram()
{
    destroy();
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept
    : id_(std::exchange(other.id_, 0))
{
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept
{
    if (this != &other) {
        destroy();
        id_ = std::exchange(other.id_, 0);
    }

    return *this;
}

void ShaderProgram::use() const
{
    glUseProgram(id_);
}

GLuint ShaderProgram::id() const noexcept
{
    return id_;
}

void ShaderProgram::setBool(std::string_view name, bool value) const
{
    use();
    glUniform1i(uniformLocation(name), value ? 1 : 0);
}

void ShaderProgram::setInt(std::string_view name, int value) const
{
    use();
    glUniform1i(uniformLocation(name), value);
}

void ShaderProgram::setFloat(std::string_view name, float value) const
{
    use();
    glUniform1f(uniformLocation(name), value);
}

void ShaderProgram::setVec2(std::string_view name, const glm::vec2& value) const
{
    use();
    glUniform2fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::setVec3(std::string_view name, const glm::vec3& value) const
{
    use();
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::setVec4(std::string_view name, const glm::vec4& value) const
{
    use();
    glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void ShaderProgram::setMat3(std::string_view name, const glm::mat3& value) const
{
    use();
    glUniformMatrix3fv(
        uniformLocation(name),
        1,
        GL_FALSE,
        glm::value_ptr(value)
    );
}

void ShaderProgram::setMat4(std::string_view name, const glm::mat4& value) const
{
    use();
    glUniformMatrix4fv(
        uniformLocation(name),
        1,
        GL_FALSE,
        glm::value_ptr(value)
    );
}


void ShaderProgram::setFLoatArr(std::string_view name, GLfloat *value, GLsizei count) const
{
    use();
    std::string n{ name };
    n += "[0]";
    glUniform1fv(uniformLocation(n), count, value);
}

void ShaderProgram::setUniformBlockBinding(std::string_view name, GLuint binding_point) const
{
    auto index = uniformBlockLocation(name);
    glUniformBlockBinding(id_, index, binding_point);
}

std::string ShaderProgram::readTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in);

    if (!file) {
        throw std::runtime_error("failed to open shader file: " + path.string());
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

GLuint ShaderProgram::compileShader(GLenum type, std::string_view source)
{
    const GLuint shader = glCreateShader(type);

    if (shader == 0) {
        throw std::runtime_error("failed to create shader object");
    }

    const char* sourcePtr = source.data();
    const GLint sourceLength = static_cast<GLint>(source.size());

    glShaderSource(shader, 1, &sourcePtr, &sourceLength);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        std::string log;
        if (logLength > 0) {
            log.resize(static_cast<std::size_t>(logLength));
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        }

        glDeleteShader(shader);

        std::string typeName;
        switch (type) {
        case GL_VERTEX_SHADER:
            typeName = "vertex shader";
            break;
        case GL_FRAGMENT_SHADER:
            typeName = "fragment shader";
            break;
        case GL_GEOMETRY_SHADER:
            typeName = "geometry shader";
            break;
        default:
            typeName = "unknown shader";
            break;
        }

        throw std::runtime_error("failed to compile " + typeName + ":\n" + log);
    }

    return shader;
}

GLuint ShaderProgram::linkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    const GLuint program = glCreateProgram();

    if (program == 0) {
        throw std::runtime_error("failed to create shader program object");
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        std::string log;
        if (logLength > 0) {
            log.resize(static_cast<std::size_t>(logLength));
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
        }

        glDeleteProgram(program);

        throw std::runtime_error("failed to link shader program:\n" + log);
    }

    return program;
}

GLint ShaderProgram::uniformLocation(std::string_view name) const
{
    const std::string nameStr(name);

    const GLint location = glGetUniformLocation(id_, nameStr.c_str());

    if (location == GL_INVALID_INDEX) {
        throw std::runtime_error("uniform not found or optimized out: " + nameStr);
    }

    return location;
}

GLuint ShaderProgram::uniformBlockLocation(std::string_view name) const
{
    const std::string nameStr(name);

    const GLuint location = glGetUniformBlockIndex(id_, nameStr.c_str());

    if (location == GL_INVALID_INDEX) {
        throw std::runtime_error("uniform not found or optimized out: " + nameStr);
    }

    return location;
}

void ShaderProgram::destroy() noexcept
{
    if (id_ != 0) {
        glDeleteProgram(id_);
        id_ = 0;
    }
}