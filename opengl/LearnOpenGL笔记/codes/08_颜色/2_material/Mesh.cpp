#include "Mesh.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &id_);
    if (id_ == 0) {
        throw std::runtime_error("Failed to generate vertex array");
    }
}

VertexArray::~VertexArray()
{
    if (id_ != 0) {
        glDeleteVertexArrays(1, &id_);
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : id_(std::exchange(other.id_, 0))
{
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other) {
        if (id_ != 0) {
            glDeleteVertexArrays(1, &id_);
        }
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

void VertexArray::bind() const
{
    glBindVertexArray(id_);
}

void VertexArray::unbind()
{
    glBindVertexArray(0);
}

GLuint VertexArray::id() const
{
    return id_;
}

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

void Buffer::unbind(GLenum target)
{
    glBindBuffer(target, 0);
}

void Buffer::setData(const void* data, std::size_t sizeBytes, GLenum usage) const
{
    bind();
    glBufferData(target_, static_cast<GLsizeiptr>(sizeBytes), data, usage);
}

GLuint Buffer::id() const
{
    return id_;
}

GLenum Buffer::target() const
{
    return target_;
}

VertexBuffer::VertexBuffer()
    : Buffer{ GL_ARRAY_BUFFER }
{
}

IndexBuffer::IndexBuffer()
    : Buffer{ GL_ELEMENT_ARRAY_BUFFER }
{
}

Mesh::Mesh(std::span<const Vertex> vertices,
           std::span<const VertexAttrib> vertexAttributes,
           std::span<const std::uint32_t> indices)
    : indexCount_(static_cast<GLsizei>(indices.size()))
{
    vao_.bind();

    vbo_.setData(
        vertices.data(),
        vertices.size_bytes(),
        GL_STATIC_DRAW
    );

    ebo_.setData(
        indices.data(),
        indices.size_bytes(),
        GL_STATIC_DRAW
    );

    setupVertexAttributes(vertexAttributes);
}

void Mesh::bind() const
{
    vao_.bind();
}

void Mesh::unbind()
{
    VertexArray::unbind();
}

void Mesh::draw() const
{
    bind();

    glDrawElements(
        GL_TRIANGLES,
        indexCount_,
        GL_UNSIGNED_INT,
        nullptr
    );
}

GLsizei Mesh::indexCount() const
{
    return indexCount_;
}

void Mesh::setupVertexAttributes(std::span<const VertexAttrib> vertexAttributes)
{
    for (auto &attrib : vertexAttributes) {
        glVertexAttribPointer(
            attrib.index,
            attrib.size,
            attrib.type,
            attrib.normalized,
            attrib.stride,
            attrib.pointer
        );
        glEnableVertexAttribArray(attrib.index);
    }
}
