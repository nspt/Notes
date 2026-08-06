#include "Buffers.h"
#include "ShaderProgram.h"
#include "glm/detail/type_mat.hpp"

#include <memory>
#include <stdexcept>
#include <array>

Buffer::Buffer(GLenum target)
{
    glGenBuffers(1, &data_->id_);
    if (data_->id_ == 0) {
        throw std::runtime_error("Failed to generate buffer");
    }
}

void Buffer::bind() const
{
    glBindBuffer(data_->target_, data_->id_);
}

void Buffer::unbind() const
{
    glBindBuffer(data_->target_, 0);
}

void Buffer::unbind(GLenum target)
{
    glBindBuffer(target, 0);
}

void Buffer::setData(const void* data, std::size_t sizeBytes, GLenum usage) const
{
    bind();
    glBufferData(data_->target_, static_cast<GLsizeiptr>(sizeBytes), data, usage);
}

void Buffer::setSubData(std::size_t offset, const void* data, std::size_t sizeBytes) const
{
    bind();
    glBufferSubData(data_->target_, offset, static_cast<GLsizeiptr>(sizeBytes), data);
}

GLuint Buffer::id() const
{
    return data_->id_;
}

GLenum Buffer::target() const
{
    return data_->target_;
}

VertexBuffer::VertexBuffer()
    : Buffer{ GL_ARRAY_BUFFER }
{
}

VertexBuffer::VertexBuffer(std::span<const Vertex> vertices)
    : VertexBuffer{}
{
    setData(vertices);
}

void VertexBuffer::setData(std::span<const Vertex> vertices)
{
    Buffer::setData(
        vertices.data(),
        vertices.size_bytes(),
        GL_STATIC_DRAW
    );
    count_ = vertices.size();
}

size_t VertexBuffer::count() const
{
    return count_;
}

IndexBuffer::IndexBuffer()
    : Buffer{ GL_ELEMENT_ARRAY_BUFFER }
{
}

IndexBuffer::IndexBuffer(std::span<const std::uint32_t> indices)
    : IndexBuffer{}
{
    setData(indices);
}

void IndexBuffer::setData(std::span<const std::uint32_t> indices)
{
    Buffer::setData(
        indices.data(),
        indices.size_bytes(),
        GL_STATIC_DRAW
    );
    count_ = indices.size();
}

size_t IndexBuffer::count() const
{
    return count_;
}

InstanceBuffer::InstanceBuffer(std::span<const glm::mat4> instances, GLenum usage)
    : Buffer{ GL_ARRAY_BUFFER }
{
    setData(instances, usage);
}

void InstanceBuffer::setData(std::span<const glm::mat4> instances, GLenum usage)
{
    Buffer::setData(
        instances.data(),
        instances.size_bytes(),
        usage
    );
    instances_ = std::vector<glm::mat4>{ instances.begin(), instances.end() };
}

void InstanceBuffer::update(size_t index, std::span<const glm::mat4> instances)
{
    if (index + instances.size() > instances_.size()) {
        throw std::out_of_range{ "Instance buffer out of range" };
    }
    for (size_t i = index; i < instances_.size(); ++i) {
        instances_[i] = instances[i];
    }
    Buffer::setSubData(index * sizeof(glm::mat4), instances.data(), instances.size_bytes());
}

const std::vector<glm::mat4> &InstanceBuffer::data() const
{
    return instances_;
}

UniformBuffer::UniformBuffer(size_t sizeBytes)
    : Buffer{ GL_UNIFORM_BUFFER }
{
    setData(nullptr, sizeBytes, GL_DYNAMIC_DRAW);
}

void UniformBuffer::setSubData(GLintptr offset, GLsizeiptr size, const void *data) const
{
    bind();
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}

void UniformBuffer::bindBase(GLuint bindingPoint) const
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, data_->id_);
}

void UniformBuffer::bindRange(GLuint bindingPoint, GLuint offset, GLuint size) const
{
    glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, data_->id_, offset, size);
}

void UniformBuffer::unbindBase(GLuint bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, 0);
}