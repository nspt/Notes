#pragma once
#include <memory>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

class ShaderProgram {
public:
    ShaderProgram() = default;
    ShaderProgram(const std::filesystem::path& vertexShaderPath,
                  const std::filesystem::path& fragmentShaderPath,
                  const std::filesystem::path& geometryShaderPath = std::filesystem::path{});

    void use() const;

    GLuint id() const noexcept;

    void setBool(GLint location, bool value) const;
    void setBool(std::string_view name, bool value) const;
    void setInt(GLint location, int value) const;
    void setInt(std::string_view name, int value) const;
    void setFloat(GLint location, float value) const;
    void setFloat(std::string_view name, float value) const;

    void setVec2(GLint location, const glm::vec2& value) const;
    void setVec2(std::string_view name, const glm::vec2& value) const;
    void setVec3(GLint location, const glm::vec3& value) const;
    void setVec3(std::string_view name, const glm::vec3& value) const;
    void setVec4(GLint location, const glm::vec4& value) const;
    void setVec4(std::string_view name, const glm::vec4& value) const;
    void setIVec4(GLint location, const glm::ivec4& value) const;
    void setIVec4(std::string_view name, const glm::ivec4& value) const;

    void setMat3(GLint location, const glm::mat3& value) const;
    void setMat3(std::string_view name, const glm::mat3& value) const;
    void setMat4(GLint location, const glm::mat4& value) const;
    void setMat4(std::string_view name, const glm::mat4& value) const;

    void setFLoatArr(GLint location, GLfloat *value, GLsizei count) const;
    void setFLoatArr(std::string_view name, GLfloat *value, GLsizei count) const;

    void setUniformBlockBinding(std::string_view name, GLuint binding_point) const;

    GLint uniformLocation(std::string_view name) const;
    GLuint uniformBlockLocation(std::string_view name) const;

    static std::string readTextFile(const std::filesystem::path& path);

    static GLuint compileShader(GLenum type, std::string_view source);
    static GLuint linkProgram(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader);

private:
    struct Data {
        GLuint id_ = 0;
    };
    std::shared_ptr<Data> data_{
        new Data{},
        [](Data *p) {
            if (p->id_ != 0)
                glDeleteProgram(p->id_);
            delete p;
        }
    };
};
