#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

struct alignas(16) DirectionalLight {
    glm::vec4 direction_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
};

struct alignas(16) PointLight {
    glm::vec4 pos_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 attenuation;   // x: constant, y: linear, z: quadratic, w: reserve
};

struct alignas(16) SpotLight {
    glm::vec4 pos_;
    glm::vec4 ambient_;
    glm::vec4 diffuse_;
    glm::vec4 specular_;
    glm::vec4 direction_inner_; // xyz:direction, w:inner cutoff
    glm::vec4 attenuation_outter_;   // x: constant, y: linear, z: quadratic, w: outer cutoff
};

struct LightData {
    glm::ivec4 counts{ 0 }; // x: directional, y: point, z: spot
    DirectionalLight directional[2];
    SpotLight spot[4];
    PointLight point[8];
};