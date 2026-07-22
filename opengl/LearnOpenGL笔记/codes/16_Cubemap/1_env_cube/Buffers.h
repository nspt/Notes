#pragma once

#include <glad/glad.h>
#include <cstdint>
#include <type_traits>
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
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    void bind() const;
    void unbind() const;
    static void unbind(GLenum target);
    void setData(const void* data, size_t sizeBytes, GLenum usage) const;
    void setSubData(size_t offset, const void* data, size_t sizeBytes) const;

    GLuint id() const;
    GLenum target() const;

protected:
    GLuint id_{ 0 };
    GLenum target_{ 0 };
};

class VertexBuffer : public Buffer {
public:
    VertexBuffer();
};

class IndexBuffer : public Buffer {
public:
    IndexBuffer();
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
    static void unbindBase(GLuint bindingPoint);
};