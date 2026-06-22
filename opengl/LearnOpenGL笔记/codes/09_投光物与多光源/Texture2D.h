#pragma once

#include <glad/glad.h>

#include <string>

class Texture2D {
public:
    Texture2D() = default;

    explicit Texture2D(const std::string& path, bool flipVertically = true);

    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void loadFromFile(const std::string& path, bool flipVertically = true);

    void bind(GLuint unit) const;
    static void unbind();

    GLuint id() const noexcept;
    int width() const noexcept;
    int height() const noexcept;
    int channels() const noexcept;
    bool valid() const noexcept;

private:
    void destroy() noexcept;

    GLuint id_ = 0;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
};
