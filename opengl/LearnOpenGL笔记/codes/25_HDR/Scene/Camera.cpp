#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>

Camera::Camera(Type type,
    const glm::vec3 &pos,
    const glm::vec3 &up,
    float yaw, float pitch)
    : type_{ type }, up_ref_{ up }, yaw_{ yaw }, pitch_{ pitch }
{
    camera_data_.pos = glm::vec4{ pos, 1.0f };
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);
    updateInfo();
}

const CameraData &Camera::cameraData() const
{
    return camera_data_;
}

void Camera::setProjection(const glm::mat4 &projection)
{
    camera_data_.projection = projection;
}

const glm::mat4 &Camera::viewMatrix() const
{
    return camera_data_.view;
}

glm::vec3 Camera::pos() const
{
    return glm::vec3{ camera_data_.pos };
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

Camera::Type Camera::type() const
{
    return type_;
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
    camera_data_.pos += glm::vec4{ vec, 0.0f };
    updateInfo();
}

void Camera::moveTo(const glm::vec3 &pos)
{
    camera_data_.pos = glm::vec4{ pos, 1.0f };
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
    const glm::vec3 eye{ camera_data_.pos };
    camera_data_.view = glm::lookAt(eye, eye + front_, up_real_);
}
