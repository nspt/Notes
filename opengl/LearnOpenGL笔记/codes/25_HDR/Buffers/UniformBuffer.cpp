#include "UniformBuffer.h"


UniformBuffer::UniformBuffer(size_t sizeBytes)
    : Buffer{ GL_UNIFORM_BUFFER }
{
    setData(nullptr, sizeBytes, GL_DYNAMIC_DRAW);
}

void UniformBuffer::setSubData(GLintptr offset, GLsizeiptr size, const void *data) const
{
    bind();
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}

void UniformBuffer::bindBase(GLuint bindingPoint) const
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, prop_->id_);
}

void UniformBuffer::bindRange(GLuint bindingPoint, GLuint offset, GLuint size) const
{
    glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, prop_->id_, offset, size);
}

void UniformBuffer::unbindBase(GLuint bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, 0);
}