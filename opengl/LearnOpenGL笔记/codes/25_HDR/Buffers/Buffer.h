#pragma once

#include <glad/glad.h>
#include <memory>

class Buffer {
public:
    explicit Buffer(GLenum target);

    void bind() const;
    void unbind() const;
    static void unbind(GLenum target);

    GLuint id() const;
    GLenum target() const;

protected:
    void setData(const void* data, size_t sizeBytes, GLenum usage) const;
    void setSubData(size_t offset, const void* data, size_t sizeBytes) const;

protected:
    struct Prop {
        GLuint id_{ 0 };
        GLenum target_{ 0 };
    };
    std::shared_ptr<Prop> prop_{
        new Prop{},
        [](Prop *p){
            if (p->id_ != 0)
                glDeleteBuffers(1, &p->id_);
            delete p;
        }
    };
};
