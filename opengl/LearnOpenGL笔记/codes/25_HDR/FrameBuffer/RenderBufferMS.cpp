#include "RenderBufferMS.h"

#include <stdexcept>

RenderBufferMS::RenderBufferMS(GLenum format, int width, int height, GLsizei samples)
{
    glGenRenderbuffers(1, &prop_->id_);
    if (prop_->id_ == 0)
        throw std::runtime_error{ "Generate buffer failed" };
    prop_->format_ = format;
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->samples_ = samples;
    glBindRenderbuffer(GL_RENDERBUFFER, prop_->id_);
    glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, prop_->samples_, prop_->format_, prop_->width_, prop_->height_
    );
}

unsigned int RenderBufferMS::id() const
{
    return prop_->id_;
}

GLsizei RenderBufferMS::samples() const
{
    return prop_->samples_;
}

void RenderBufferMS::reallocate(GLenum format, int width, int height, GLsizei samples)
{
    prop_->format_ = format;
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->samples_ = samples;
    glBindRenderbuffer(GL_RENDERBUFFER, prop_->id_);
    glRenderbufferStorageMultisample(
        GL_RENDERBUFFER, prop_->samples_, prop_->format_, prop_->width_, prop_->height_
    );
}
