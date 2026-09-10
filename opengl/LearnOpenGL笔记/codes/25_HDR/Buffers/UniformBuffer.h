#pragma once

#include "Buffer.h"

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