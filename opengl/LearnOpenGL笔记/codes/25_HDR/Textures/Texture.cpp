#include "Texture.h"
#include <format>
#include <stdexcept>

namespace {

GLuint g_active_unit{ 0 };

enum class TargetIndex {
    Texture1D,
    Texture2D,
    Texture3D,
    CubeMap,
    Texture1DArray,
    Texture2DArray,
    Texture2DMultisample,
    Texture2DMultisampleArray,

    Count
};

constexpr int kMaxTextureUnits = 64;
GLuint g_bound_ids[kMaxTextureUnits][static_cast<int>(TargetIndex::Count)]{};

TargetIndex targetIndex(GLenum target)
{
    switch (target) {
    case GL_TEXTURE_1D:
        return TargetIndex::Texture1D;
    case GL_TEXTURE_2D:
        return TargetIndex::Texture2D;
    case GL_TEXTURE_3D:
        return TargetIndex::Texture3D;
    case GL_TEXTURE_CUBE_MAP:
        return TargetIndex::CubeMap;
    case GL_TEXTURE_1D_ARRAY:
        return TargetIndex::Texture1DArray;
    case GL_TEXTURE_2D_ARRAY:
        return TargetIndex::Texture2DArray;
    case GL_TEXTURE_2D_MULTISAMPLE:
        return TargetIndex::Texture2DMultisample;
    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
        return TargetIndex::Texture2DMultisampleArray;
    default:
        throw std::logic_error{ std::format("Unsupported texture target: {}", target) };
    }
}

void ensureActiveUnit(GLuint unit)
{
    if (g_active_unit == unit)
        return;
    glActiveTexture(GL_TEXTURE0 + unit);
    g_active_unit = unit;
}

GLuint &cachedBoundId(GLuint unit, GLenum target)
{
    if (unit >= static_cast<GLuint>(kMaxTextureUnits)) {
        throw std::logic_error{ std::format("Texture unit out of range: {}", unit) };
    }
    return g_bound_ids[unit][static_cast<int>(targetIndex(target))];
}

} // namespace

Texture::Texture(GLenum target)
{
    prop_->target_ = target;
    glGenTextures(1, &prop_->id_);
    if (prop_->id_ == 0)
        throw std::runtime_error{ "Generate texture failed" };
}

void Texture::setWrapMode(GLint wrapS, GLint wrapT, std::optional<GLint> wrapR) const
{
    bind(0);
    glTexParameteri(prop_->target_, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(prop_->target_, GL_TEXTURE_WRAP_T, wrapT);
    if (wrapR.has_value()) {
        glTexParameteri(prop_->target_, GL_TEXTURE_WRAP_R, wrapR.value());
    }
}

void Texture::bind(GLuint unit) const
{
    ensureActiveUnit(unit);

    auto &cached_id = cachedBoundId(unit, prop_->target_);
    if (cached_id == prop_->id_)
        return;

    glBindTexture(prop_->target_, prop_->id_);
    cached_id = prop_->id_;
}

void Texture::unbind(GLuint unit) const
{
    ensureActiveUnit(unit);

    auto &cached_id = cachedBoundId(unit, prop_->target_);
    if (cached_id == 0)
        return;

    glBindTexture(prop_->target_, 0);
    cached_id = 0;
}

GLuint Texture::id() const noexcept
{
    return prop_->id_;
}

int Texture::width() const noexcept
{
    return prop_->width_;
}

int Texture::height() const noexcept
{
    return prop_->height_;
}

GLenum Texture::format() const noexcept
{
    return prop_->format_;
}

GLenum Texture::target() const noexcept
{
    return prop_->target_;
}

bool Texture::valid() const noexcept
{
    return prop_->id_ != 0;
}
