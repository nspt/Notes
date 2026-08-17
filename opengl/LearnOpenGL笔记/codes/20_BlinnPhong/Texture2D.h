#pragma once
#include "Texture.h"
#include <string>

class Texture2D : public Texture {
public:
    Texture2D(int width, int height, GLenum format, void *data = nullptr);
    explicit Texture2D(const std::string& path, bool flipVertically = true);

    void generateMipmap() const;
    void reallocate(int width, int height, GLenum format, void *data = nullptr);

private:
    void init(void *data, int width, int height, GLenum format) noexcept;
};
