#pragma once

#include "Texture.h"
#include <memory>
#include <map>

class RenderBuffer {
public:
    RenderBuffer(GLenum format, int width, int height)
        : format_{ format }, width_{ width }, height_{ height }
    {
        glGenRenderbuffers(1, &id_);
        glBindRenderbuffer(GL_RENDERBUFFER, id_);
        glRenderbufferStorage(GL_RENDERBUFFER, format_, width_, height_);
    }

    ~RenderBuffer()
    {
        glDeleteRenderbuffers(1, &id_);
    }

    unsigned int id() const
    {
        return id_;
    }
private:
    unsigned int id_ = 0;
    GLenum format_ = 0;
    int width_ = 0;
    int height_ = 0;
};

class FrameBuffer {
public:
    FrameBuffer()
    {
        glGenFramebuffers(1, &id_);
    }

    ~FrameBuffer()
    {
        glDeleteFramebuffers(1, &id_);
    }

    void attachTexture(GLenum attachment, std::shared_ptr<Texture> texture)
    {
        textures_[attachment] = texture;
        bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture->id(), 0);
    }

    void attachRBO(GLenum attachment, std::shared_ptr<RenderBuffer> rbo)
    {
        render_buffers_[attachment] = rbo;
        bind();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo->id());
    }

    bool isCompleted() const
    {
        bind();
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    void bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, id_);
    }

    static void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    unsigned int id_ = 0;
    std::map<GLenum, std::shared_ptr<Texture>> textures_;
    std::map<GLenum, std::shared_ptr<RenderBuffer>> render_buffers_;
};