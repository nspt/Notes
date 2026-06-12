#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Light {
public:
    glm::vec3 pos_;
    glm::vec3 ambient_;
    glm::vec3 diffuse_;
    glm::vec3 specular_;
};