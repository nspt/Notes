#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>

Camera::Camera(
    const glm::vec3 &pos,
    const glm::vec3 &up,
    float yaw, float pitch)
    : pos_{ pos }, up_ref_{ up }, yaw_{ yaw }, pitch_{ pitch }
{
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    updateInfo();
}

const glm::mat4 &Camera::viewMatrix() const
{
    return view_matrix_;
}

const glm::vec3 &Camera::pos() const
{
    return pos_;
}

const glm::vec3 &Camera::front() const
{
    return front_;
}

const glm::vec3 &Camera::right() const
{
    return right_;
}

const glm::vec3 &Camera::up() const
{
    return up_real_;
}


float Camera::yaw() const
{
    return yaw_;
}

float Camera::pitch() const
{
    return pitch_;
}

void Camera::yaw(float deg)
{
    yaw_ += deg;
    updateInfo();
}

void Camera::pitch(float deg)
{
    pitch_ = std::clamp(pitch_ + deg, -89.0f, 89.0f);
    updateInfo();
}

void Camera::move(const glm::vec3 &vec)
{
    pos_ += vec;
    updateInfo();
}

void Camera::moveTo(const glm::vec3 &pos)
{
    pos_ = pos;
    updateInfo();
}

void Camera::updateInfo()
{
    front_.x = -sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_.y = sin(glm::radians(pitch_));
    front_.z = -cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_ = glm::normalize(front_);
    right_ = glm::normalize(glm::cross(front_, up_ref_));
    up_real_ = glm::normalize(glm::cross(right_, front_));
    view_matrix_ = glm::lookAt(pos_, pos_ + front_, up_real_);
}
