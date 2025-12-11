#pragma once
#include <glad/glad.h>
#include <glm/vec3.hpp>

#include "Shader.hpp"

struct DirLight {
    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct PointLight {
    glm::vec3 position;
    float constant;
    float linear;
    float quadratic;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

class LightManager {
public:
    DirLight dirLight;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;

    void send(const Shader& shader) const;
};
