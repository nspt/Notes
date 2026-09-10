#include "InstanceBuffer.h"

#include <stdexcept>

#include <glm/gtc/matrix_transform.hpp>

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