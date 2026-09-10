#pragma once

#include <glad/glad.h>
#include <memory>

class RenderBuffer {
public:
    RenderBuffer(GLenum format, int width, int height);

    unsigned int id() const;
    void reallocate(GLenum format, int width, int height);

private:
    struct Prop {
        unsigned int id_ = 0;
        GLenum format_ = 0;
        int width_ = 0;
        int height_ = 0;
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
