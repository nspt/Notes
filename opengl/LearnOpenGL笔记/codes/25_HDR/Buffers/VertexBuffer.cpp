#include "VertexBuffer.h"

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