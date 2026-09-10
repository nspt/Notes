#pragma once
#include "Texture.h"
#include <string>

class Texture2D : public Texture {
public:
    Texture2D() : Texture{ GL_TEXTURE_2D } {}
    Texture2D(int width, int height, GLenum internalFormat, GLenum format,
              void *data = nullptr, GLenum type = GL_UNSIGNED_BYTE);
    explicit Texture2D(const std::string& path, bool flipVertically = true, bool srgb = true);

    void generateMipmap() const;
    void reallocate(int width, int height, GLenum internalFormat, GLenum format,
                    void *data = nullptr, GLenum type = GL_UNSIGNED_BYTE);

private:
    void init(void *data, int width, int height, GLenum internalFormat, GLenum format,
              GLenum type = GL_UNSIGNED_BYTE) noexcept;
};
