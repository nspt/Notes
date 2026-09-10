#include "Mesh.h"

#include <memory>
#include <stdexcept>
#include <utility>
#include <array>

VertexArray::VertexArray()
{
    GLuint id;
    glGenVertexArrays(1, &data_->id_);
    if (data_->id_ == 0) {
        throw std::runtime_error("Failed to generate vertex array");
    }
}


void VertexArray::bind() const
{
    glBindVertexArray(data_->id_);
}

void VertexArray::unbind()
{
    glBindVertexArray(0);
}

GLuint VertexArray::id() const
{
    return data_->id_;
}

Mesh::Mesh()
{
    vao_.bind();
    ebo_.bind();
    setupVertexAttributes(vbo_);
    setupInstanceAttributes(ibo_);
    vao_.unbind();
}

Mesh::Mesh(VertexBuffer vbo, IndexBuffer ebo, std::optional<InstanceBuffer> ibo)
    : vbo_{ std::move(vbo) }, ebo_{ std::move(ebo) },
    ibo_{ ibo.has_value() ?
        std::move(ibo.value()) :
        InstanceBuffer{ std::vector<InstanceBuffer::InstanceData>{ {} } }
    }
{
    vao_.bind();
    ebo_.bind();
    setupVertexAttributes(vbo_);
    setupInstanceAttributes(ibo_);
    vao_.unbind();
}


Mesh::Mesh(const Mesh &rhs)
    : Mesh{ rhs.vbo_, rhs.ebo_ }
{
    setInstanceBuffer(rhs.ibo_.data());
}

Mesh& Mesh::operator=(const Mesh &rhs)
{
    Mesh tmp{ rhs };
    *this = std::move(tmp);
    return *this;
}

void Mesh::bind() const
{
    vao_.bind();
}

void Mesh::unbind()
{
    VertexArray::unbind();
}

void Mesh::draw(GLsizei instances) const
{
    if (static_cast<size_t>(instances) > ibo_.data().size()) {
        throw std::logic_error{ "Instances too many" };
    }
    vao_.bind();
    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(ebo_.count()),
        GL_UNSIGNED_INT,
        nullptr,
        instances
    );
    vao_.unbind();
}

void Mesh::draw(const InstanceBuffer &ibo) const
{
    vao_.bind();
    setupInstanceAttributes(ibo);
    glDrawElementsInstanced(
        GL_TRIANGLES,
        static_cast<GLsizei>(ebo_.count()),
        GL_UNSIGNED_INT,
        nullptr,
        static_cast<GLsizei>(ibo.data().size())
    );
    setupInstanceAttributes(ibo_);
    vao_.unbind();
}

void Mesh::setInstanceBuffer(InstanceBuffer ibo)
{
    vao_.bind();
    ibo_ = std::move(ibo);
    setupInstanceAttributes(ibo_);
    vao_.unbind();
}

size_t Mesh::instanceCount() const
{
    return ibo_.count();
}

const VertexBuffer &Mesh::vertexBuffer() const
{
    return vbo_;
}

const IndexBuffer &Mesh::indexBuffer() const
{
    return ebo_;
}

const InstanceBuffer &Mesh::instanceBuffer() const
{
    return ibo_;
}

void Mesh::setupVertexAttributes(const VertexBuffer &vbo) const
{
    vbo.bind();
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, texCoord)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, tangent)));
    glEnableVertexAttribArray(3);
}

void Mesh::setupInstanceAttributes(const InstanceBuffer &ibo) const
{
    ibo.bind();
    std::size_t vec4Size = sizeof(glm::vec4);
    // a_model occupies locations 4..7 (mat4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)0);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(1 * vec4Size));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));

    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);
    glVertexAttribDivisor(7, 1);
}
