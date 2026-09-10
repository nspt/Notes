#pragma once

#include <span>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Buffer.h"

struct Vertex {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
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
