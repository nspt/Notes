#pragma once
#include "Texture.h"
#include <string>
#include <vector>

class TextureCubeMap : public Texture {
public:
    TextureCubeMap() : Texture{ GL_TEXTURE_CUBE_MAP } {}
    TextureCubeMap(int width, int height, GLenum internalFormat, GLenum format,
                   GLenum type = GL_UNSIGNED_BYTE);
    explicit TextureCubeMap(const std::vector<std::string>& paths, bool flipVertically = true, bool srgb = true);

private:
    void init(const std::vector<void*> faces, int width, int height,
              GLenum internalFormat, GLenum format, GLenum type = GL_UNSIGNED_BYTE) noexcept;
};
