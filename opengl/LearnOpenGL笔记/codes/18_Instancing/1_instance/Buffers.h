#pragma once

#include <glad/glad.h>
#include <initializer_list>
#include <memory>
#include <span>
#include <vector>
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

class Buffer {
public:
    explicit Buffer(GLenum target);

    void bind() const;
    void unbind() const;
    static void unbind(GLenum target);

    GLuint id() const;
    GLenum target() const;

protected:
    void setData(const void* data, size_t sizeBytes, GLenum usage) const;
    void setSubData(size_t offset, const void* data, size_t sizeBytes) const;

protected:
    struct Data {
        GLuint id_{ 0 };
        GLenum target_{ 0 };
    };
    std::shared_ptr<Data> data_{
        new Data{},
        [](Data *p){
            if (p->id_ != 0)
                glDeleteBuffers(1, &p->id_);
            delete p;
        }
    };
};

class VertexBuffer : public Buffer {
public:
    VertexBuffer();
    VertexBuffer(std::span<const Vertex> vertices);

    void setData(std::span<const Vertex> vertices);
    size_t count() const;
private:
    size_t count_{ 0 };
};

class IndexBuffer : public Buffer {
public:
    IndexBuffer();
    IndexBuffer(std::span<const std::uint32_t> indices);

    void setData(std::span<const std::uint32_t> indices);
    size_t count() const;
private:
    size_t count_{ 0 };
};

class InstanceBuffer : public Buffer {
public:
    struct InstanceData {
        glm::vec3 traslation_{ 0 };
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

class UniformBuffer : public Buffer {
public:
    template<typename T>
    UniformBuffer(const T& data)
        : Buffer{ GL_UNIFORM_BUFFER }
    {
        setData(reinterpret_cast<const void*>(&data), sizeof(T), GL_DYNAMIC_DRAW);
    }
    UniformBuffer(size_t sizeBytes);

    void bindBase(GLuint bindingPoint) const;
    void bindRange(GLuint bindingPoint, GLuint offset, GLuint size) const;
    void setSubData(GLintptr offset, GLsizeiptr size, const void *data) const;
    static void unbindBase(GLuint bindingPoint);
};