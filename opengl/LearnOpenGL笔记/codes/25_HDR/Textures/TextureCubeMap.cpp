#include "TextureCubeMap.h"
#include "Texture.h"

#include <stb_image.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>


using namespace std;

TextureCubeMap::TextureCubeMap(const vector<string>& paths, bool flipVertically, bool srgb)
    : Texture{ GL_TEXTURE_CUBE_MAP }
{
    if (paths.size() != 6) {
        throw std::logic_error{ "Cubemap must use 6 face textures" };
    }
    stbi_set_flip_vertically_on_load(flipVertically);
    int width, height, channel;

    auto stbi_deleter = [](unsigned char *p) {
        stbi_image_free(p);
    };
    using img_ptr = std::unique_ptr<unsigned char, decltype(stbi_deleter)>;
    vector<img_ptr> face_imgs;
    for(unsigned int i = 0; i < paths.size(); i++) {
        int w, h, c;
        img_ptr data{ stbi_load(paths[i].c_str(), &w, &h, &c, 0), stbi_deleter };
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + paths[i]);
        }
        if (i == 0) {
            width = w;
            height = h;
            channel = c;
        } else if (width != w || height != h || channel != c) {
            throw std::runtime_error("Failed to load texture: " + paths[i] + ", size or channel unmatch");
        }
        face_imgs.push_back(std::move(data));
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

    vector<void*> faces;
    for (auto &f : face_imgs) {
        faces.push_back(f.get());
    }
    reallocate(faces, width, height, internalFormat, format);
    generateMipmap();
}

TextureCubeMap::TextureCubeMap(int width, int height, GLenum internalFormat, GLenum format, GLenum type)
    : Texture{ GL_TEXTURE_CUBE_MAP }
{
    reallocate({}, width, height, internalFormat, format, type);
}

void TextureCubeMap::generateMipmap() const
{
    bind(0);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void TextureCubeMap::reallocate(const std::vector<void*> faces, int width, int height,
                                GLenum internalFormat, GLenum format, GLenum type)
{
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->format_ = internalFormat;

    bind(0);

    const unsigned face_count = faces.empty() ? 6u : static_cast<unsigned>(faces.size());
    for (unsigned int i = 0; i < face_count; i++) {
        void *data = faces.empty() ? nullptr : faces[i];
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, internalFormat, width, height, 0, format, type, data
        );
    }
}
