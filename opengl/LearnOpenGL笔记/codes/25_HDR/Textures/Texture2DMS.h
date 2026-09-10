#pragma once
#include "Texture.h"

class Texture2DMS : public Texture {
public:
    Texture2DMS() : Texture{ GL_TEXTURE_2D_MULTISAMPLE } {}
    Texture2DMS(int width, int height, GLenum internalFormat, GLsizei samples,
                GLboolean fixedSampleLocations = GL_TRUE);

    void reallocate(int width, int height, GLenum internalFormat, GLsizei samples,
                    GLboolean fixedSampleLocations = GL_TRUE);
    GLsizei samples() const noexcept;

private:
    void init(int width, int height, GLenum internalFormat, GLsizei samples,
              GLboolean fixedSampleLocations) noexcept;

    struct MSProp {
        GLsizei samples_{ 0 };
        GLboolean fixed_sample_locations_{ GL_TRUE };
    };
    std::shared_ptr<MSProp> ms_prop_{ std::make_shared<MSProp>() };
};
