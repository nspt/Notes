#pragma once
#include "Texture.h"
#include <string>

class Texture2D : public Texture {
public:
    Texture2D(int width, int height, GLenum format);
    explicit Texture2D(const std::string& path, bool flipVertically = true);

private:
    void init(void *data, int width, int height, GLenum format) noexcept;
};
