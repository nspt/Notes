#pragma once

#include "Texture.h"
#include <memory>
#include <map>
#include <stdexcept>

class RenderBuffer {
public:
    RenderBuffer(GLenum format, int width, int height)
    {
        glGenRenderbuffers(1, &data_->id_);
        if (data_->id_ == 0)
            throw std::runtime_error{ "Generate buffer failed" };
        data_->format_ = format;
        data_->width_ = width;
        data_->height_ = height;
        glBindRenderbuffer(GL_RENDERBUFFER, data_->id_);
        glRenderbufferStorage(GL_RENDERBUFFER, data_->format_, data_->width_, data_->height_);
    }

    unsigned int id() const
    {
        return data_->id_;
    }
private:
    struct Data {
        unsigned int id_ = 0;
        GLenum format_ = 0;
        int width_ = 0;
        int height_ = 0;
    };
    std::shared_ptr<Data> data_{
        new Data{},
        [](Data *p) {
            if (p->id_ != 0)
                glDeleteRenderbuffers(1, &p->id_);
            delete p;
        }
    };
};

class FrameBuffer {
public:
    FrameBuffer()
    {
        glGenFramebuffers(1, &data_->id_);
        if (data_->id_ == 0)
            throw std::runtime_error{ "Generate buffer failed" };
    }

    void attachTexture(GLenum attachment, Texture texture)
    {
        data_->textures_.insert_or_assign(attachment, texture);
        bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.id(), 0);
    }

    void attachRBO(GLenum attachment, RenderBuffer rbo)
    {
        data_->render_buffers_.insert_or_assign(attachment, rbo);
        bind();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, rbo.id());
    }

    bool isCompleted() const
    {
        bind();
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    void bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, data_->id_);
    }

    static void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    struct Data {
        unsigned int id_ = 0;
        std::map<GLenum, Texture> textures_;
        std::map<GLenum, RenderBuffer> render_buffers_;
    };
    std::shared_ptr<Data> data_{
        new Data{},
        [](Data *p) {
            if (p->id_ != 0)
                glDeleteFramebuffers(1, &p->id_);
            delete p;
        }
    };
};