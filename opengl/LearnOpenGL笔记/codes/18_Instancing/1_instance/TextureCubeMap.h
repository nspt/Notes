#pragma once
#include "Texture.h"
#include <string>
#include <vector>

class TextureCubeMap : public Texture {
public:
    TextureCubeMap(int width, int height, GLenum format);
    explicit TextureCubeMap(const std::vector<std::string>& paths, bool flipVertically = true);

private:
    void init(const std::vector<void*> faces, int width, int height, GLenum format) noexcept;
};
