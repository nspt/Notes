#include "Texture2DMS.h"

Texture2DMS::Texture2DMS(int width, int height, GLenum internalFormat, GLsizei samples,
                         GLboolean fixedSampleLocations)
    : Texture{ GL_TEXTURE_2D_MULTISAMPLE }
{
    init(width, height, internalFormat, samples, fixedSampleLocations);
}

void Texture2DMS::reallocate(int width, int height, GLenum internalFormat, GLsizei samples,
                             GLboolean fixedSampleLocations)
{
    data_->width_ = width;
    data_->height_ = height;
    data_->format_ = internalFormat;
    samples_ = samples;
    fixed_sample_locations_ = fixedSampleLocations;

    bind(0);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        samples_,
        internalFormat,
        data_->width_,
        data_->height_,
        fixed_sample_locations_
    );
}

GLsizei Texture2DMS::samples() const noexcept
{
    return samples_;
}

void Texture2DMS::init(int width, int height, GLenum internalFormat, GLsizei samples,
                       GLboolean fixedSampleLocations) noexcept
{
    data_->width_ = width;
    data_->height_ = height;
    data_->format_ = internalFormat;
    samples_ = samples;
    fixed_sample_locations_ = fixedSampleLocations;

    glGenTextures(1, &data_->id_);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, data_->id_);

    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        samples_,
        internalFormat,
        data_->width_,
        data_->height_,
        fixed_sample_locations_
    );
}
