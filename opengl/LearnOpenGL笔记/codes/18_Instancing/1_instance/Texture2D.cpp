#include "Texture2D.h"
#include "Texture.h"
#include <stb_image.h>
#include <stdexcept>
#include <string>
#include <memory>

Texture2D::Texture2D(const std::string& path, bool flipVertically)
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
    if (channel == 1) {
        format = GL_RED;
    } else if (channel == 3) {
        format = GL_RGB;
    } else if (channel == 4) {
        format = GL_RGBA;
    } else {
        throw std::runtime_error("Unsupported texture channel count: " + std::to_string(channel));
    }
    init(data.get(), width, height, format);
}

Texture2D::Texture2D(int width, int height, GLenum format)
    : Texture{ GL_TEXTURE_2D }
{
    init(nullptr, width, height, format);
}

void Texture2D::init(void *data, int width, int height, GLenum format) noexcept
{
    data_->width_ = width;
    data_->height_ = height;
    data_->format_ = format;

    glGenTextures(1, &data_->id_);
    glBindTexture(GL_TEXTURE_2D, data_->id_);

    glTexImage2D(GL_TEXTURE_2D, 0,
        data_->format_, data_->width_, data_->height_, 0,
        data_->format_, GL_UNSIGNED_BYTE, data
    );
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
