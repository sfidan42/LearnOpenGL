#pragma once
#include <glad/glad.h>
#include <glm/vec3.hpp>

#include "Shader.hpp"

class Light
{
public:
	glm::vec3 position;
	glm::vec3 color;

	Light();
	~Light();

	void send(const Shader& shader) const
	{
		shader.set3f("light.position", position.r, position.g, position.b);
		shader.set3f("light.color", color.r, color.g, color.b);
		shader.set3f("light.ambient",  0.2f, 0.2f, 0.2f);
		shader.set3f("light.diffuse",  0.5f, 0.5f, 0.5f); // darken diffuse light a bit
		shader.set3f("light.specular", 1.0f, 1.0f, 1.0f);
	}

	void bind() const;

private:
	GLuint VAO;
};
