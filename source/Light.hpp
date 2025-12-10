#pragma once
#include <glad/glad.h>
#include <glm/vec3.hpp>

#include "Shader.hpp"

class Light
{
public:
	glm::vec3 position{1.2f, 1.0f, 2.0f};
	glm::vec3 ambient{0.2f, 0.2f, 0.2f};
	glm::vec3 diffuse{0.5f, 0.5f, 0.5f};
	glm::vec3 specular = {1.0f, 1.0f, 1.0f};

	Light();
	~Light();

	void bind() const;
	void send(const Shader& shader) const;

private:
	GLuint VAO = -1;
};
