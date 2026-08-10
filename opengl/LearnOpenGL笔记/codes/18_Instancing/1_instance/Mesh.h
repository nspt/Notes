#pragma once

#include "Buffers.h"
#include <memory>
#include <optional>

class VertexArray {
public:
    VertexArray();

    void bind() const;
    static void unbind();

    GLuint id() const;

private:
    struct Data {
        GLuint id_{ 0 };
    };
    std::shared_ptr<Data> data_{
        new Data{},
        [](Data *p) {
            if (p->id_ != 0)
                glDeleteVertexArrays(1, &p->id_);
            delete p;
        }
    };
};

class Mesh {
public:
    Mesh();
    Mesh(VertexBuffer vbo, IndexBuffer ebo, std::optional<InstanceBuffer> ibo = std::nullopt);

    Mesh(const Mesh &rhs);
    Mesh(Mesh &&rhs) = default;

    Mesh& operator=(const Mesh &rhs);
    Mesh& operator=(Mesh &&rhs) = default;

    void bind() const;
    static void unbind();
    void draw(GLsizei count = 1) const;
    void draw(const InstanceBuffer &ibo) const;

    const VertexBuffer &vertexBuffer() const;

    const IndexBuffer &indexBuffer() const;

    void setInstanceBuffer(InstanceBuffer ibo);
    const InstanceBuffer &instanceBuffer() const;
    size_t instanceCount() const;

private:
    void setupVertexAttributes(const VertexBuffer &vbo) const;
    void setupInstanceAttributes(const InstanceBuffer &ibo) const;

    VertexArray vao_;
    VertexBuffer vbo_;
    IndexBuffer ebo_;
    InstanceBuffer ibo_;
};
