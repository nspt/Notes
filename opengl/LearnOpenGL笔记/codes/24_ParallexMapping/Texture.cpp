#include "Texture.h"
#include <utility>

Texture::Texture(GLenum target)
{
    data_->target_ = target;
}

void Texture::setWrapMode(GLint wrapS, GLint wrapT, std::optional<GLint> wrapR) const
{
    bind(0);
    glTexParameteri(data_->target_, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(data_->target_, GL_TEXTURE_WRAP_T, wrapT);
    if (wrapR.has_value()) {
        glTexParameteri(data_->target_, GL_TEXTURE_WRAP_R, wrapR.value());
    }
}

void Texture::bind(GLuint unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(data_->target_, data_->id_);
}

void Texture::unbind()
{
    glBindTexture(data_->target_, 0);
}

GLuint Texture::id() const noexcept
{
    return data_->id_;
}

int Texture::width() const noexcept
{
    return data_->width_;
}

int Texture::height() const noexcept
{
    return data_->height_;
}

GLenum Texture::format() const noexcept
{
    return data_->format_;
}

GLenum Texture::target() const noexcept
{
    return data_->target_;
}

bool Texture::valid() const noexcept
{
    return data_->id_ != 0;
}
