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
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->format_ = internalFormat;
    ms_prop_->samples_ = samples;
    ms_prop_->fixed_sample_locations_ = fixedSampleLocations;

    bind(0);
    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        ms_prop_->samples_,
        internalFormat,
        prop_->width_,
        prop_->height_,
        ms_prop_->fixed_sample_locations_
    );
}

GLsizei Texture2DMS::samples() const noexcept
{
    return ms_prop_->samples_;
}

void Texture2DMS::init(int width, int height, GLenum internalFormat, GLsizei samples,
                       GLboolean fixedSampleLocations) noexcept
{
    prop_->width_ = width;
    prop_->height_ = height;
    prop_->format_ = internalFormat;
    ms_prop_->samples_ = samples;
    ms_prop_->fixed_sample_locations_ = fixedSampleLocations;

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, prop_->id_);

    glTexImage2DMultisample(
        GL_TEXTURE_2D_MULTISAMPLE,
        ms_prop_->samples_,
        internalFormat,
        prop_->width_,
        prop_->height_,
        ms_prop_->fixed_sample_locations_
    );
}
