#pragma once
#include "Shader.hpp"

struct Material
{
	void send(const Shader& shader) const;

	float shininess{32.0f};
	glm::vec3 ambient{1.0f, 0.5f, 0.31f};
	glm::vec3 diffuse{1.0f, 0.5f, 0.31f};
	glm::vec3 specular{0.5f, 0.5f, 0.5f};
};
