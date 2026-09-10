#pragma once
#include <memory>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <map>
#include <type_traits>

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
    void setMat4Arr(GLint location, const glm::mat4 *values, GLsizei count) const;
    void setMat4Arr(std::string_view name, const glm::mat4 *values, GLsizei count) const;

    void setFLoatArr(GLint location, GLfloat *value, GLsizei count) const;
    void setFLoatArr(std::string_view name, GLfloat *value, GLsizei count) const;

    void setUniformBlockBinding(std::string_view name, GLuint binding_point) const;

    GLint uniformLocation(std::string_view name) const;
    GLuint uniformBlockLocation(std::string_view name) const;

    // uniform 不存在或被优化掉时静默跳过，返回是否成功写入
    template<typename T>
    bool setUniformIfPresent(std::string_view name, const T &value) const
    {
        const GLint location = uniformLocation(name);
        if (location < 0) {
            return false;
        }
        if constexpr (std::is_same_v<T, bool>) {
            setBool(location, value);
        } else if constexpr (std::is_same_v<T, int>) {
            setInt(location, value);
        } else if constexpr (std::is_same_v<T, float>) {
            setFloat(location, value);
        } else if constexpr (std::is_same_v<T, glm::vec2>) {
            setVec2(location, value);
        } else if constexpr (std::is_same_v<T, glm::vec3>) {
            setVec3(location, value);
        } else if constexpr (std::is_same_v<T, glm::vec4>) {
            setVec4(location, value);
        } else if constexpr (std::is_same_v<T, glm::ivec4>) {
            setIVec4(location, value);
        } else if constexpr (std::is_same_v<T, glm::mat3>) {
            setMat3(location, value);
        } else if constexpr (std::is_same_v<T, glm::mat4>) {
            setMat4(location, value);
        } else {
            static_assert(sizeof(T) == 0, "unsupported uniform type");
        }
        return true;
    }

    static std::string readTextFile(const std::filesystem::path& path);
    static std::string resolveIncludes(const std::string &source,
                                       const std::filesystem::path &base_dir,
                                       int depth = 0);

    static GLuint compileShader(GLenum type, std::string_view source,
                                const std::filesystem::path& path = {});
    static GLuint linkProgram(GLuint vertexShader, GLuint geometryShader, GLuint fragmentShader,
                              const std::filesystem::path& vertexPath = {},
                              const std::filesystem::path& fragmentPath = {},
                              const std::filesystem::path& geometryPath = {});

private:
    struct Data {
        GLuint id_ = 0;
        mutable std::map<std::string, GLint> uniform_loc_cache_;
        mutable std::map<std::string, GLint> uniform_block_loc_cache_;
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
