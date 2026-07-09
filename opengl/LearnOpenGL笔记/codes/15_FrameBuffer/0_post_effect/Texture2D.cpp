#include "Texture2D.h"

#include <stb_image.h>

#include <stdexcept>
#include <string>
#include <utility>

Texture2D::Texture2D(const std::string& path, bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically);
    int channel;
    unsigned char* data = stbi_load(
        path.c_str(),
        &width_,
        &height_,
        &channel,
        0
    );

    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    if (channel == 1) {
        format_ = GL_RED;
    } else if (channel == 3) {
        format_ = GL_RGB;
    } else if (channel == 4) {
        format_ = GL_RGBA;
    } else {
        stbi_image_free(data);
        throw std::runtime_error("Unsupported texture channel count: " + std::to_string(channel));
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexImage2D(GL_TEXTURE_2D, 0,
        format_, width_, height_, 0,
        format_, GL_UNSIGNED_BYTE, data
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

Texture2D::Texture2D(int width, int height, GLenum format)
    : width_{ width }, height_{ height }, format_{ format }
{
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexImage2D(GL_TEXTURE_2D, 0,
        format_, width_, height_, 0,
        format_, GL_UNSIGNED_BYTE, nullptr
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture2D::~Texture2D()
{
    destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : id_{ std::exchange(other.id_, 0) }
    , width_{ std::exchange(other.width_, 0) }
    , height_{ std::exchange(other.height_, 0) }
    , format_{ std::exchange(other.format_, 0) }
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other) {
        destroy();
        id_ = std::exchange(other.id_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        format_ = std::exchange(other.format_, 0);
    }

    return *this;
}

void Texture2D::setWrapMode(GLint wrapS, GLint wrapT) const
{
    bind(0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
}

void Texture2D::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void Texture2D::unbind()
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture2D::id() const noexcept
{
    return id_;
}

int Texture2D::width() const noexcept
{
    return width_;
}

int Texture2D::height() const noexcept
{
    return height_;
}

GLenum Texture2D::format() const noexcept
{
    return format_;
}

bool Texture2D::valid() const noexcept
{
    return id_ != 0;
}

void Texture2D::destroy() noexcept
{
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
}
