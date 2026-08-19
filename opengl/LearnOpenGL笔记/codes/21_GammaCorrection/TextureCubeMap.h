#pragma once
#include "Texture.h"
#include <string>
#include <vector>

class TextureCubeMap : public Texture {
public:
    TextureCubeMap(int width, int height, GLenum internalFormat, GLenum format);
    TextureCubeMap(int width, int height, GLenum format);
    explicit TextureCubeMap(const std::vector<std::string>& paths, bool flipVertically = true, bool srgb = true);

private:
    void init(const std::vector<void*> faces, int width, int height, GLenum internalFormat, GLenum format) noexcept;
};
