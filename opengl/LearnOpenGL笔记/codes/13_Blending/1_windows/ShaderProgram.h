#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <string>
#include <string_view>

class ShaderProgram {
public:
    ShaderProgram() = default;

    ShaderProgram(const std::filesystem::path& vertexShaderPath,
                  const std::filesystem::path& fragmentShaderPath);

    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const;

    GLuint id() const noexcept;

    void setBool(std::string_view name, bool value) const;
    void setInt(std::string_view name, int value) const;
    void setFloat(std::string_view name, float value) const;

    void setVec2(std::string_view name, const glm::vec2& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;

    void setMat3(std::string_view name, const glm::mat3& value) const;
    void setMat4(std::string_view name, const glm::mat4& value) const;

    void setUniformBlockBinding(std::string_view name, GLuint binding_point) const;

    GLint uniformLocation(std::string_view name) const;
    GLuint uniformBlockLocation(std::string_view name) const;

    static std::string readTextFile(const std::filesystem::path& path);

    static GLuint compileShader(GLenum type, std::string_view source);
    static GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);

private:
    void destroy() noexcept;

    GLuint id_ = 0;
};