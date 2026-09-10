#pragma once

#include <initializer_list>
#include <span>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Buffer.h"

class InstanceBuffer : public Buffer {
public:
    struct InstanceData {
        glm::vec3 translation_{ 0 };
        glm::quat rotation_{ 1, 0, 0, 0 };
        glm::vec3 scale_{ 1 };
    };

    InstanceBuffer();
    InstanceBuffer(const InstanceData &instance, GLenum usage = GL_STATIC_DRAW);
    InstanceBuffer(std::initializer_list<const InstanceData> instances, GLenum usage = GL_STATIC_DRAW);
    InstanceBuffer(std::span<const InstanceData> instances, GLenum usage = GL_STATIC_DRAW);
    InstanceBuffer(std::vector<InstanceData> instances, GLenum usage = GL_STATIC_DRAW);

    void setData(std::vector<InstanceData> instances, GLenum usage = GL_STATIC_DRAW);
    void update(size_t index, std::span<const InstanceData> instances);
    const std::vector<InstanceData> &data() const;
    size_t count() const;
private:
    std::vector<InstanceData> instances_;
};
