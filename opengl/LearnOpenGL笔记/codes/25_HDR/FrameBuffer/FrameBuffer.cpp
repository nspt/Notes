#include "FrameBuffer.h"

#include <format>
#include <stdexcept>
#include <utility>

namespace {

GLuint g_prev_read_fb_id{ 0 };
GLuint g_prev_draw_fb_id{ 0 };

} // namespace

FrameBuffer::FrameBuffer()
{
    glGenFramebuffers(1, &data_->id_);
    if (data_->id_ == 0)
        throw std::runtime_error{ "Generate buffer failed" };
}

void FrameBuffer::attachTexture(GLenum attachment, Texture2D texture)
{
    const GLuint id = texture.id();
    data_->attachments_.insert_or_assign(attachment, std::move(texture));
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, id, 0);
}

void FrameBuffer::attachTexture(GLenum attachment, Texture2DMS texture)
{
    const GLuint id = texture.id();
    data_->attachments_.insert_or_assign(attachment, std::move(texture));
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D_MULTISAMPLE, id, 0);
}

void FrameBuffer::attachTexture(GLenum attachment, TextureCubeMap texture)
{
    const GLuint id = texture.id();
    data_->attachments_.insert_or_assign(attachment, std::move(texture));
    bind();
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, id, 0);
}

void FrameBuffer::attachTexture(GLenum attachment, TextureCubeMap texture, GLenum cube_face)
{
    const GLuint id = texture.id();
    data_->attachments_.insert_or_assign(attachment, std::move(texture));
    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, cube_face, id, 0);
}

void FrameBuffer::attachRBO(GLenum attachment, RenderBuffer rbo)
{
    const GLuint id = rbo.id();
    data_->attachments_.insert_or_assign(attachment, std::move(rbo));
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, id);
}

void FrameBuffer::attachRBO(GLenum attachment, RenderBufferMS rbo)
{
    const GLuint id = rbo.id();
    data_->attachments_.insert_or_assign(attachment, std::move(rbo));
    bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, id);
}

std::optional<FrameBuffer::Attachment> FrameBuffer::attachment(GLenum attachment) const
{
    const auto it = data_->attachments_.find(attachment);
    if (it == data_->attachments_.end())
        return std::nullopt;
    return it->second;
}

bool FrameBuffer::isCompleted() const
{
    bind();
    return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
}

unsigned int FrameBuffer::id() const
{
    return data_->id_;
}

void FrameBuffer::bind(GLenum target) const
{
    switch (target) {
    case GL_READ_FRAMEBUFFER:
        if (data_->id_ == g_prev_read_fb_id)
            return;
        g_prev_read_fb_id = data_->id_;
        break;
    case GL_DRAW_FRAMEBUFFER:
        if (data_->id_ == g_prev_draw_fb_id)
            return;
        g_prev_draw_fb_id = data_->id_;
        break;
    case GL_FRAMEBUFFER:
        if (data_->id_ == g_prev_read_fb_id && data_->id_ == g_prev_draw_fb_id)
            return;
        g_prev_read_fb_id = data_->id_;
        g_prev_draw_fb_id = data_->id_;
        break;
    default:
        throw std::logic_error{ std::format("Unknown framebuffer bind target: {}", target) };
    }
    glBindFramebuffer(target, data_->id_);
}

void FrameBuffer::unbind(GLenum target)
{
    switch (target) {
    case GL_READ_FRAMEBUFFER:
        if (g_prev_read_fb_id == 0)
            return;
        g_prev_read_fb_id = 0;
        break;
    case GL_DRAW_FRAMEBUFFER:
        if (g_prev_draw_fb_id == 0)
            return;
        g_prev_draw_fb_id = 0;
        break;
    case GL_FRAMEBUFFER:
        if (g_prev_read_fb_id == 0 && g_prev_draw_fb_id == 0)
            return;
        g_prev_read_fb_id = 0;
        g_prev_draw_fb_id = 0;
        break;
    default:
        throw std::logic_error{ std::format("Unknown framebuffer bind target: {}", target) };
    }
    glBindFramebuffer(target, 0);
}

void FrameBuffer::readBuffer(GLenum mode)
{
    bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(mode);
}

void FrameBuffer::drawBuffer(GLenum buf)
{
    bind(GL_DRAW_FRAMEBUFFER);
    glDrawBuffer(buf);
}

void FrameBuffer::drawBuffers(GLsizei n, const GLenum *bufs)
{
    bind(GL_DRAW_FRAMEBUFFER);
    glDrawBuffers(n, bufs);
}
