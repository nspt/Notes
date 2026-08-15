#include "TextureCubeMap.h"
#include "Texture.h"

#include <stb_image.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>


using namespace std;

TextureCubeMap::TextureCubeMap(const vector<string>& paths, bool flipVertically)
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
    if (channel == 1) {
        format = GL_RED;
    } else if (channel == 3) {
        format = GL_RGB;
    } else if (channel == 4) {
        format = GL_RGBA;
    } else {
        throw std::runtime_error("Unsupported texture channel count: " + std::to_string(channel));
    }

    vector<void*> faces;
    for (auto &f : face_imgs) {
        faces.push_back(f.get());
    }
    init(faces, width, height, format);
}

TextureCubeMap::TextureCubeMap(int width, int height, GLenum format)
    : Texture{ GL_TEXTURE_CUBE_MAP }
{
    init({}, width, height, format);
}

void TextureCubeMap::init(const std::vector<void*> faces, int width, int height, GLenum format) noexcept
{
    data_->width_ = width;
    data_->height_ = height;
    data_->format_ = format;

    glGenTextures(1, &data_->id_);
    glBindTexture(GL_TEXTURE_CUBE_MAP, data_->id_);

    for(unsigned int i = 0; i < faces.size(); i++)
    {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
            0, data_->format_, width, height, 0, data_->format_, GL_UNSIGNED_BYTE, faces[i]
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); 
}
