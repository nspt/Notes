#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define MAX_DIRECTIONAL_LIGHT 2
#define MAX_POINT_LIGHT 8
#define MAX_SPOT_LIGHT 4

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
    glm::vec4 direction_inner_; // xyz:direction
    glm::vec4 attenuation_outter_;   // x: constant, y: linear, z: quadratic
    glm::vec4 cutoff_; // x: inner, y: outer, z: inverse epsilon
};

struct alignas(16) LightData {
    glm::ivec4 counts{ 0 }; // x: directional, y: point, z: spot
    DirectionalLight directional[MAX_DIRECTIONAL_LIGHT];
    SpotLight spot[MAX_SPOT_LIGHT];
    PointLight point[MAX_POINT_LIGHT];
};
