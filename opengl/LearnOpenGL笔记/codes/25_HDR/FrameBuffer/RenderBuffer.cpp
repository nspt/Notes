#include "RenderBuffer.h"

#include <stdexcept>

RenderBuffer::RenderBuffer(GLenum format, int width, int height)
{
    glGenRenderbuffers(1, &prop_->id_);
    if (prop_->id_ == 0)
        throw std::runtime_error{ "Generate buffer failed" };
    prop_->format_ = format;
    prop_->width_ = width;
    prop_->height_ = height;
    glBindRenderbuffer(GL_RENDERBUFFER, prop_->id_);
    glRenderbufferStorage(GL_RENDERBUFFER, prop_->format_, prop_->width_, prop_->height_);
}

unsigned int RenderBuffer::id() const
{
    return prop_->id_;
}

void RenderBuffer::reallocate(GLenum format, int width, int height)
{
    prop_->format_ = format;
    prop_->width_ = width;
    prop_->height_ = height;
    glBindRenderbuffer(GL_RENDERBUFFER, prop_->id_);
    glRenderbufferStorage(GL_RENDERBUFFER, prop_->format_, prop_->width_, prop_->height_);
}
