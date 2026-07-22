#include "Buffers.h"

#include <stdexcept>
#include <utility>

Buffer::Buffer(GLenum target)
    : target_{ target }
{
    glGenBuffers(1, &id_);
    if (id_ == 0) {
        throw std::runtime_error("Failed to generate buffer");
    }
}

Buffer::~Buffer()
{
    if (id_ != 0) {
        glDeleteBuffers(1, &id_);
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : id_{ std::exchange(other.id_, 0) }
    , target_{ std::exchange(other.target_, 0) }
{
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other) {
        if (id_ != 0) {
            glDeleteBuffers(1, &id_);
        }
        id_ = std::exchange(other.id_, 0);
        target_ = std::exchange(other.target_, 0);
    }
    return *this;
}

void Buffer::bind() const
{
    glBindBuffer(target_, id_);
}

void Buffer::unbind() const
{
    glBindBuffer(target_, 0);
}

void Buffer::unbind(GLenum target)
{
    glBindBuffer(target, 0);
}

void Buffer::setData(const void* data, std::size_t sizeBytes, GLenum usage) const
{
    bind();
    glBufferData(target_, static_cast<GLsizeiptr>(sizeBytes), data, usage);
}

void Buffer::setSubData(std::size_t offset, const void* data, std::size_t sizeBytes) const
{
    bind();
    glBufferSubData(target_, offset, static_cast<GLsizeiptr>(sizeBytes), data);
}

GLuint Buffer::id() const
{
    return id_;
}

GLenum Buffer::target() const
{
    return target_;
}

UniformBuffer::UniformBuffer(size_t sizeBytes)
    : Buffer{ GL_UNIFORM_BUFFER }
{
    setData(nullptr, sizeBytes, GL_DYNAMIC_DRAW);
}

void UniformBuffer::bindBase(GLuint bindingPoint) const
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, id_);
}

void UniformBuffer::bindRange(GLuint bindingPoint, GLuint offset, GLuint size) const
{
    glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, id_, offset, size);
}

void UniformBuffer::unbindBase(GLuint bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, 0);
}