#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <cstdint>
#include <span>

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
};

struct VertexAttrib {
    GLuint index;
    GLint size;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const void *pointer;
};

class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    VertexArray(VertexArray&& other) noexcept;
    VertexArray& operator=(VertexArray&& other) noexcept;

    void bind() const;
    static void unbind();

    GLuint id() const;

private:
    GLuint id_{ 0 };
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
    static void unbind(GLenum target);
    void setData(const void* data, std::size_t sizeBytes, GLenum usage) const;

    GLuint id() const;
    GLenum target() const;

private:
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

class Mesh {
public:
    Mesh(std::span<const Vertex> vertices,
         std::span<const VertexAttrib> vertexAttributes,
         std::span<const std::uint32_t> indices);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&&) noexcept = default;
    Mesh& operator=(Mesh&&) noexcept = default;

    void bind() const;
    static void unbind();
    void draw() const;

    GLsizei indexCount() const;

private:
    void setupVertexAttributes(std::span<const VertexAttrib> vertexAttributes);

    VertexArray vao_;
    VertexBuffer vbo_;
    IndexBuffer ebo_;
    GLsizei indexCount_{ 0 };
};
