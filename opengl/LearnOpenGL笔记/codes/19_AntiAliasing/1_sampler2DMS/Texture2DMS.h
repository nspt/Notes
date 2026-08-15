#pragma once
#include "Texture.h"

class Texture2DMS : public Texture {
public:
    Texture2DMS(int width, int height, GLenum format, GLsizei samples,
                GLboolean fixedSampleLocations = GL_TRUE);

    void reallocate(int width, int height, GLenum format, GLsizei samples,
                    GLboolean fixedSampleLocations = GL_TRUE);
    GLsizei samples() const noexcept;

private:
    void init(int width, int height, GLenum format, GLsizei samples,
              GLboolean fixedSampleLocations) noexcept;

    GLsizei samples_{ 0 };
    GLboolean fixed_sample_locations_{ GL_TRUE };
};
