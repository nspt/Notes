#include "ShaderProgram.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

ShaderProgram::ShaderProgram(const std::filesystem::path& vertexShaderPath,
                             const std::filesystem::path& fragmentShaderPath,
                             const std::filesystem::path& geometryShaderPath)
{
    const bool has_geom = !geometryShaderPath.empty();
    const std::string vertexSource = readTextFile(vertexShaderPath);
    const std::string geometrySource = has_geom ? readTextFile(geometryShaderPath) : "";
    const std::string fragmentSource = readTextFile(fragmentShaderPath);

    GLuint vertex_shader{ 0 }, geometry_shader{ 0 }, fragment_shader{ 0 }, program{ 0 };
    try {
        vertex_shader = compileShader(GL_VERTEX_SHADER, vertexSource);
        geometry_shader  = has_geom ? compileShader(GL_GEOMETRY_SHADER, geometrySource) : 0;
        fragment_shader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    
        data_->id_ = linkProgram(vertex_shader, geometry_shader, fragment_shader);

        glDeleteShader(vertex_shader);
        if (has_geom) {
            glDeleteShader(geometry_shader);
        }
        glDeleteShader(fragment_shader);
    } catch (...) {
        if (vertex_shader != 0) {
            glDeleteShader(vertex_shader);
        }
        if (geometry_shader != 0) {
            glDeleteShader(geometry_shader);
        }
        if (fragment_shader != 0) {
            glDeleteShader(fragment_shader);
        }
        if (program != 0) {
            glDeleteProgram(program);
        }
        throw;
    }
}

void ShaderProgram::use() const
{
    const GLuint id = data_->id_;
    if (s_cache_valid_ && s_cached_id_ == id) {
        return;
    }
    glUseProgram(id);
    s_cached_id_ = id;
    s_cache_valid_ = true;
}

void ShaderProgram::invalidateCachedProgram() noexcept
{
    s_cache_valid_ = false;
}

GLuint ShaderProgram::id() const noexcept
{
    return data_->id_;
}

void ShaderProgram::setBool(GLint location, bool value) const
{
    use();
    glUniform1i(location, value ? 1 : 0);
}

void ShaderProgram::setBool(std::string_view name, bool value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setBool(location, value);
}

void ShaderProgram::setInt(GLint location, int value) const
{
    use();
    glUniform1i(location, value);
}

void ShaderProgram::setInt(std::string_view name, int value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setInt(location, value);
}

void ShaderProgram::setFloat(GLint location, float value) const
{
    use();
    glUniform1f(location, value);
}

void ShaderProgram::setFloat(std::string_view name, float value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setFloat(location, value);
}

void ShaderProgram::setVec2(GLint location, const glm::vec2& value) const
{
    use();
    glUniform2fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setVec2(std::string_view name, const glm::vec2& value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setVec2(location, value);
}

void ShaderProgram::setVec3(GLint location, const glm::vec3& value) const
{
    use();
    glUniform3fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setVec3(std::string_view name, const glm::vec3& value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setVec3(location, value);
}

void ShaderProgram::setVec4(GLint location, const glm::vec4& value) const
{
    use();
    glUniform4fv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setVec4(std::string_view name, const glm::vec4& value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setVec4(location, value);
}

void ShaderProgram::setIVec4(GLint location, const glm::ivec4& value) const
{
    use();
    glUniform4iv(location, 1, glm::value_ptr(value));
}

void ShaderProgram::setIVec4(std::string_view name, const glm::ivec4& value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setIVec4(location, value);
}

void ShaderProgram::setMat3(GLint location, const glm::mat3& value) const
{
    use();
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setMat3(std::string_view name, const glm::mat3& value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setMat3(location, value);
}

void ShaderProgram::setMat4(GLint location, const glm::mat4& value) const
{
    use();
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::setMat4(std::string_view name, const glm::mat4& value) const
{
    const GLint location = uniformLocation(name);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string(name));
    }
    setMat4(location, value);
}

void ShaderProgram::setMat4Arr(GLint location, const glm::mat4 *values, GLsizei count) const
{
    use();
    glUniformMatrix4fv(location, count, GL_FALSE, glm::value_ptr(values[0]));
}

void ShaderProgram::setMat4Arr(std::string_view name, const glm::mat4 *values, GLsizei count) const
{
    std::string n{ name };
    n += "[0]";
    const GLint location = uniformLocation(n);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + n);
    }
    setMat4Arr(location, values, count);
}

void ShaderProgram::setFLoatArr(GLint location, GLfloat *value, GLsizei count) const
{
    use();
    glUniform1fv(location, count, value);
}

void ShaderProgram::setFLoatArr(std::string_view name, GLfloat *value, GLsizei count) const
{
    std::string n{ name };
    n += "[0]";
    const GLint location = uniformLocation(n);
    if (location < 0) {
        throw std::runtime_error("uniform not found or optimized out: " + n);
    }
    setFLoatArr(location, value, count);
}

void ShaderProgram::setUniformBlockBinding(std::string_view name, GLuint binding_point) const
{
    auto index = uniformBlockLocation(name);
    if (index == GL_INVALID_INDEX) {
        throw std::runtime_error("uniform not found or optimized out: " + std::string{ name });
    }
    glUniformBlockBinding(data_->id_, index, binding_point);
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

GLuint ShaderProgram::linkProgram(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader)
{
    const GLuint program = glCreateProgram();

    if (program == 0) {
        throw std::runtime_error("failed to create shader program object");
    }

    glAttachShader(program, vertexShader);
    if (geometryShader != 0) {
        glAttachShader(program, geometryShader);
    }
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

        throw std::runtime_error("failed to link shader program: " + log);
    }

    return program;
}

GLint ShaderProgram::uniformLocation(std::string_view name) const
{
    return glGetUniformLocation(data_->id_, std::string(name).c_str());
}

GLuint ShaderProgram::uniformBlockLocation(std::string_view name) const
{
    const std::string nameStr(name);

    return glGetUniformBlockIndex(data_->id_, nameStr.c_str());
}
