#pragma once

#include <glad/glad.h>
#include <memory>

class RenderBufferMS {
public:
    RenderBufferMS(GLenum format, int width, int height, GLsizei samples);

    unsigned int id() const;
    GLsizei samples() const;
    void reallocate(GLenum format, int width, int height, GLsizei samples);

private:
    struct Prop {
        unsigned int id_ = 0;
        GLenum format_ = 0;
        int width_ = 0;
        int height_ = 0;
        GLsizei samples_ = 0;
    };
    std::shared_ptr<Prop> prop_{
        new Prop{},
        [](Prop *p) {
            if (p->id_ != 0)
                glDeleteRenderbuffers(1, &p->id_);
            delete p;
        }
    };
};
