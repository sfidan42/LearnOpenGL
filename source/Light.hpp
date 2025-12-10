#pragma once
#include <glad/glad.h>
#include <glm/vec3.hpp>

class Light
{
public:
	glm::vec3 color;

	Light();
	~Light();

	void bind() const;

private:
	GLuint VAO;
};
