#pragma once

#include "RenderBuffer.h"
#include "RenderBufferMS.h"
#include "../Textures/Texture2D.h"
#include "../Textures/Texture2DMS.h"
#include "../Textures/TextureCubeMap.h"
#include <memory>
#include <map>
#include <optional>
#include <variant>

class FrameBuffer {
public:
    using Attachment = std::variant<Texture2D, Texture2DMS, TextureCubeMap, RenderBuffer, RenderBufferMS>;

    FrameBuffer();

    void attachTexture(GLenum attachment, Texture2D texture);
    void attachTexture(GLenum attachment, Texture2DMS texture);
    void attachTexture(GLenum attachment, TextureCubeMap texture);
    void attachTexture(GLenum attachment, TextureCubeMap texture, GLenum cube_face);

    void attachRBO(GLenum attachment, RenderBuffer rbo);
    void attachRBO(GLenum attachment, RenderBufferMS rbo);

    std::optional<Attachment> attachment(GLenum attachment) const;

    bool isCompleted() const;
    unsigned int id() const;
    void bind(GLenum target = GL_FRAMEBUFFER) const;
    static void unbind(GLenum target = GL_FRAMEBUFFER);

    void readBuffer(GLenum mode);
    void drawBuffer(GLenum buf);
    void drawBuffers(GLsizei n, const GLenum *bufs);

private:
    struct Data {
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
