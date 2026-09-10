#pragma once

#include <glad/glad.h>
#include <memory>
#include <optional>

class Texture {
public:
    Texture(GLenum target);

    void bind(GLuint unit) const;
    void unbind(GLuint unit) const;

    GLuint id() const noexcept;
    int width() const noexcept;
    int height() const noexcept;
    GLenum format() const noexcept;
    GLenum target() const noexcept;
    bool valid() const noexcept;

    void setWrapMode(GLint wrapS, GLint wrapT, std::optional<GLint> wrapR = std::nullopt) const;

    bool operator==(const Texture &rhs) const noexcept = default;
protected:
    struct Prop {
        GLuint id_ = 0;
        GLint width_ = 0;
        GLint height_ = 0;
        GLenum format_ = 0;
        GLenum target_ = 0;
    };
    std::shared_ptr<Prop> prop_{
        new Prop{},
        [](Prop *p){
            if (p->id_ != 0)
                glDeleteTextures(1, &p->id_);
            delete p;
        }
    };
};
