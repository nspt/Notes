#pragma once

#include "Buffers.h"
#include <span>

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
