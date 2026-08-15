#pragma once

#include "Texture2D.h"
#include "Texture2DMS.h"
#include <memory>
#include <map>
#include <stdexcept>
#include <variant>

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

    void reallocate(GLenum format, int width, int height)
    {
        data_->format_ = format;
        data_->width_ = width;
        data_->height_ = height;
        glBindRenderbuffer(GL_RENDERBUFFER, data_->id_);
        glRenderbufferStorage(GL_RENDERBUFFER, data_->format_, data_->width_, data_->height_);
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

class RenderBufferMS {
public:
    RenderBufferMS(GLenum format, int width, int height, GLsizei samples)
    {
        glGenRenderbuffers(1, &data_->id_);
        if (data_->id_ == 0)
            throw std::runtime_error{ "Generate buffer failed" };
        data_->format_ = format;
        data_->width_ = width;
        data_->height_ = height;
        data_->samples_ = samples;
        glBindRenderbuffer(GL_RENDERBUFFER, data_->id_);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER, data_->samples_, data_->format_, data_->width_, data_->height_
        );
    }

    unsigned int id() const
    {
        return data_->id_;
    }

    GLsizei samples() const
    {
        return data_->samples_;
    }

    void reallocate(GLenum format, int width, int height, GLsizei samples)
    {
        data_->format_ = format;
        data_->width_ = width;
        data_->height_ = height;
        data_->samples_ = samples;
        glBindRenderbuffer(GL_RENDERBUFFER, data_->id_);
        glRenderbufferStorageMultisample(
            GL_RENDERBUFFER, data_->samples_, data_->format_, data_->width_, data_->height_
        );
    }

private:
    struct Data {
        unsigned int id_ = 0;
        GLenum format_ = 0;
        int width_ = 0;
        int height_ = 0;
        GLsizei samples_ = 0;
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

    void attachTexture(GLenum attachment, Texture2D texture)
    {
        const GLuint id = texture.id();
        data_->attachments_.insert_or_assign(attachment, std::move(texture));
        bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, id, 0);
    }

    void attachTexture(GLenum attachment, Texture2DMS texture)
    {
        const GLuint id = texture.id();
        data_->attachments_.insert_or_assign(attachment, std::move(texture));
        bind();
        glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, id, 0);
    }

    void attachRBO(GLenum attachment, RenderBuffer rbo)
    {
        const GLuint id = rbo.id();
        data_->attachments_.insert_or_assign(attachment, std::move(rbo));
        bind();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, id);
    }

    void attachRBO(GLenum attachment, RenderBufferMS rbo)
    {
        const GLuint id = rbo.id();
        data_->attachments_.insert_or_assign(attachment, std::move(rbo));
        bind();
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, id);
    }

    bool isCompleted() const
    {
        bind();
        return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }

    unsigned int id() const
    {
        return data_->id_;
    }

    void bind(GLenum target = GL_FRAMEBUFFER) const
    {
        glBindFramebuffer(target, data_->id_);
    }

    static void unbind(GLenum target = GL_FRAMEBUFFER)
    {
        glBindFramebuffer(target, 0);
    }

private:
    struct Data {
        using Attachment = std::variant<Texture2D, Texture2DMS, RenderBuffer, RenderBufferMS>;
        unsigned int id_ = 0;
        std::map<GLenum, Attachment> attachments_;
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
