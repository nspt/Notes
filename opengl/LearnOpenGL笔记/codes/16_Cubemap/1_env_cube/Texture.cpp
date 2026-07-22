#include "Texture.h"
#include <utility>

Texture::Texture(GLenum target)
    : target_{ target }
{}

Texture::~Texture()
{
    destroy();
}

Texture::Texture(Texture&& other) noexcept
    : id_{ std::exchange(other.id_, 0) }
    , width_{ std::exchange(other.width_, 0) }
    , height_{ std::exchange(other.height_, 0) }
    , format_{ std::exchange(other.format_, 0) }
    , target_{ std::exchange(other.target_, 0)}
{
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        destroy();
        id_ = std::exchange(other.id_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        format_ = std::exchange(other.format_, 0);
        target_ = std::exchange(other.target_, 0);
    }

    return *this;
}

void Texture::setWrapMode(GLint wrapS, GLint wrapT, std::optional<GLint> wrapR) const
{
    bind(0);
    glTexParameteri(target_, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(target_, GL_TEXTURE_WRAP_T, wrapT);
    if (wrapR.has_value()) {
        glTexParameteri(target_, GL_TEXTURE_WRAP_R, wrapR.value());
    }
}

void Texture::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(target_, id_);
}

void Texture::unbind()
{
    glBindTexture(target_, 0);
}

GLuint Texture::id() const noexcept
{
    return id_;
}

int Texture::width() const noexcept
{
    return width_;
}

int Texture::height() const noexcept
{
    return height_;
}

GLenum Texture::format() const noexcept
{
    return format_;
}

GLenum Texture::target() const noexcept
{
    return target_;
}

bool Texture::valid() const noexcept
{
    return id_ != 0;
}

void Texture::destroy() noexcept
{
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
}
