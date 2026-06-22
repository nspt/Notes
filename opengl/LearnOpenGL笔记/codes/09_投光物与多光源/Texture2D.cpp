#include "Texture2D.h"

#include <stb_image.h>

#include <stdexcept>
#include <string>
#include <utility>

Texture2D::Texture2D(const std::string& path, bool flipVertically)
{
    loadFromFile(path, flipVertically);
}

Texture2D::~Texture2D()
{
    destroy();
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : id_(std::exchange(other.id_, 0))
    , width_(std::exchange(other.width_, 0))
    , height_(std::exchange(other.height_, 0))
    , channels_(std::exchange(other.channels_, 0))
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other) {
        destroy();

        id_ = std::exchange(other.id_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        channels_ = std::exchange(other.channels_, 0);
    }

    return *this;
}

void Texture2D::loadFromFile(const std::string& path, bool flipVertically)
{
    destroy();

    stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(
        path.c_str(),
        &width_,
        &height_,
        &channels_,
        0
    );

    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    GLenum dataFormat = GL_RGB;
    GLenum internalFormat = GL_RGB;

    if (channels_ == 1) {
        dataFormat = GL_RED;
        internalFormat = GL_RED;
    } else if (channels_ == 3) {
        dataFormat = GL_RGB;
        internalFormat = GL_RGB;
    } else if (channels_ == 4) {
        dataFormat = GL_RGBA;
        internalFormat = GL_RGBA;
    } else {
        stbi_image_free(data);
        throw std::runtime_error("Unsupported texture channel count: " + std::to_string(channels_));
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        internalFormat,
        width_,
        height_,
        0,
        dataFormat,
        GL_UNSIGNED_BYTE,
        data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    glBindTexture(GL_TEXTURE_2D, 0);
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

int Texture2D::channels() const noexcept
{
    return channels_;
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
