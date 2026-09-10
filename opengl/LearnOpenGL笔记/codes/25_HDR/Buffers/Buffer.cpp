#include "Buffer.h"

#include <stdexcept>

Buffer::Buffer(GLenum target)
{
    prop_->target_ = target;
    glGenBuffers(1, &prop_->id_);
    if (prop_->id_ == 0) {
        throw std::runtime_error("Failed to generate buffer");
    }
}

void Buffer::bind() const
{
    glBindBuffer(prop_->target_, prop_->id_);
}

void Buffer::unbind() const
{
    glBindBuffer(prop_->target_, 0);
}

void Buffer::unbind(GLenum target)
{
    glBindBuffer(target, 0);
}

void Buffer::setData(const void* data, std::size_t sizeBytes, GLenum usage) const
{
    bind();
    glBufferData(prop_->target_, static_cast<GLsizeiptr>(sizeBytes), data, usage);
}

void Buffer::setSubData(std::size_t offset, const void* data, std::size_t sizeBytes) const
{
    bind();
    glBufferSubData(prop_->target_, offset, static_cast<GLsizeiptr>(sizeBytes), data);
}

GLuint Buffer::id() const
{
    return prop_->id_;
}

GLenum Buffer::target() const
{
    return prop_->target_;
}
