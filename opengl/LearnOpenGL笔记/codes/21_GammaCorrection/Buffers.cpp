#include "Buffers.h"
#include "ShaderProgram.h"
#include "glm/detail/type_mat.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

#include <memory>
#include <stdexcept>
#include <array>
#include <vector>

Buffer::Buffer(GLenum target)
{
    data_->target_ = target;
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

InstanceBuffer::InstanceBuffer()
    : Buffer{ GL_ARRAY_BUFFER }
{}

InstanceBuffer::InstanceBuffer(const InstanceData &instance, GLenum usage)
    : InstanceBuffer{ std::vector<InstanceData>{ instance }, usage }
{}

InstanceBuffer::InstanceBuffer(std::initializer_list<const InstanceData> instances, GLenum usage)
    : InstanceBuffer{ std::vector<InstanceData>{ instances.begin(), instances.end() }, usage }
{}

InstanceBuffer::InstanceBuffer(std::span<const InstanceData> instances, GLenum usage)
    : InstanceBuffer{ std::vector<InstanceData>{ instances.begin(), instances.end() }, usage }
{}

InstanceBuffer::InstanceBuffer(std::vector<InstanceData> instances, GLenum usage)
    : Buffer{ GL_ARRAY_BUFFER }
{
    setData(instances, usage);
}

void InstanceBuffer::setData(std::vector<InstanceData> instances, GLenum usage)
{
    std::vector<glm::mat4> models;
    models.reserve(instances.size());
    for (auto &instance : instances) {
        glm::mat4 m{ 1.0f };
        m = glm::translate(m, instance.translation_);
        m = m * glm::mat4_cast(instance.rotation_);
        m = glm::scale(m, instance.scale_);
        models.push_back(m);
    }
    Buffer::setData(
        models.data(),
        models.size() * sizeof(models[0]),
        usage
    );
    instances_ = std::move(instances);
}

void InstanceBuffer::update(size_t index, std::span<const InstanceData> instances)
{
    if (index + instances.size() > instances_.size()) {
        throw std::out_of_range{ "Instance buffer out of range" };
    }
    std::vector<glm::mat4> models;
    models.reserve(instances.size());
    for (auto &instance : instances) {
        glm::mat4 m{ 1.0f };
        m = glm::translate(m, instance.translation_);
        m = m * glm::mat4_cast(instance.rotation_);
        m = glm::scale(m, instance.scale_);
        models.push_back(m);
    }
    Buffer::setSubData(index * sizeof(models[0]), models.data(), models.size() * sizeof(models[0]));

    for (size_t i = index; i < instances_.size(); ++i) {
        instances_[i] = instances[i];
    }
    
}

const std::vector<InstanceBuffer::InstanceData> &InstanceBuffer::data() const
{
    return instances_;
}

size_t InstanceBuffer::count() const
{
    return instances_.size();
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