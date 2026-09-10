#include "Texture2D.h"
#include "Texture.h"
#include <stb_image.h>
#include <stdexcept>
#include <string>
#include <memory>

Texture2D::Texture2D(const std::string& path, bool flipVertically, bool srgb)
    : Texture{ GL_TEXTURE_2D }
{
    stbi_set_flip_vertically_on_load(flipVertically);
    int width, height, channel;

    auto stbi_deleter = [](unsigned char *p) {
        stbi_image_free(p);
    };
    std::unique_ptr<unsigned char, decltype(stbi_deleter)> data{
        stbi_load(path.c_str(), &width, &height, &channel, 0),
        stbi_deleter
    };

    if (!data) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    GLenum format;
    GLenum internalFormat;
    if (channel == 1) {
        format = GL_RED;
        internalFormat = GL_RED;
    } else if (channel == 3) {
        format = GL_RGB;
        internalFormat = srgb ? GL_SRGB : GL_RGB;
    } else if (channel == 4) {
        format = GL_RGBA;
        internalFormat = srgb ? GL_SRGB_ALPHA : GL_RGBA;
    } else {
        throw std::runtime_error("Unsupported texture channel count: " + std::to_string(channel));
    }
    init(data.get(), width, height, internalFormat, format);
}

Texture2D::Texture2D(int width, int height, GLenum internalFormat, GLenum format, void *data, GLenum type)
    : Texture{ GL_TEXTURE_2D }
{
    init(data, width, height, internalFormat, format, type);
}

void Texture2D::generateMipmap() const
{
    bind(0);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::reallocate(int width, int height, GLenum internalFormat, GLenum format, void *data, GLenum type)
{
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->format_ = internalFormat;

    bind(0);
    glTexImage2D(GL_TEXTURE_2D, 0,
        prop_->format_, prop_->width_, prop_->height_, 0,
        format, type, data
    );
}

void Texture2D::init(void *data, int width, int height, GLenum internalFormat, GLenum format, GLenum type) noexcept
{
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->format_ = internalFormat;

    glBindTexture(GL_TEXTURE_2D, prop_->id_);

    glTexImage2D(GL_TEXTURE_2D, 0,
        prop_->format_, prop_->width_, prop_->height_, 0,
        format, type, data
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
