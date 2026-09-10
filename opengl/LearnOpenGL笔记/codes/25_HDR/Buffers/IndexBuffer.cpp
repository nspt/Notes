#include "IndexBuffer.h"

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