#pragma once

#include <glad/glad.h>
#include <memory>
#include <span>
#include <vector>
#include "glm/glm.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct VertexAttrib {
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const void *pointer;
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
    InstanceBuffer(std::span<const glm::mat4> instances = std::vector<glm::mat4>{ glm::mat4{ 1.0f } }, GLenum usage = GL_STATIC_DRAW);

    void setData(std::span<const glm::mat4> instances, GLenum usage = GL_STATIC_DRAW);
    void update(size_t index, std::span<const glm::mat4> instances);
    const std::vector<glm::mat4> &data() const;
private:
    std::vector<glm::mat4> instances_;
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