#pragma once

#include "glm/detail/type_vec.hpp"
#include <glm/glm.hpp>

class Camera
{
public:
    enum class Type {
        Fly, FPS
    };
    Camera(Type type = Type::Fly,
        const glm::vec3 &pos = glm::vec3{ 0.0f, 0.0f, 0.0f },
        const glm::vec3 &up = glm::vec3{ 0.0f, 1.0f, 0.0f },
        float yaw = 0.0f, float pitch = 0.0f
    );

    const glm::mat4 &viewMatrix() const;
    const glm::vec3 &pos() const;
    const glm::vec3 &front() const;
    const glm::vec3 &right() const;
    const glm::vec3 &up() const;
    float yaw() const;
    float pitch() const;
    Type type() const;

    void yaw(float deg);
    void pitch(float deg);
    void move(const glm::vec3 &vec);
    void moveTo(const glm::vec3 &pos);
private:
    void updateInfo();

    float yaw_{ 0.0f };
    float pitch_{ 0.0f };
    glm::vec3 pos_{ 0.0f, 0.0f, 0.0f };
    glm::vec3 front_{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right_{ 1.0f, 0.0f, 0.0f };
    glm::vec3 up_ref_{ 0.0f, 1.0f, 0.0f };
    glm::vec3 up_real_{ 0.0f, 1.0f, 0.0f };
    glm::mat4 view_matrix_{ 1.0f };

    Type type_{ Type::Fly };
};

struct CameraData {
    glm::mat4 view;
    glm::mat4 projection;
    glm::vec4 pos;
};