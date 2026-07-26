#pragma once

#include <glad/glad.h>
#include <optional>

class Texture {
public:
    Texture(GLenum target);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    void bind(GLuint unit) const;
    void unbind();

    GLuint id() const noexcept;
    int width() const noexcept;
    int height() const noexcept;
    GLenum format() const noexcept;
    GLenum target() const noexcept;
    bool valid() const noexcept;

    void setWrapMode(GLint wrapS, GLint wrapT, std::optional<GLint> wrapR = std::nullopt) const;

protected:
    void destroy() noexcept;

    GLuint id_ = 0;
    GLint width_ = 0;
    GLint height_ = 0;
    GLenum format_ = 0;
    GLenum target_ = 0;
};
