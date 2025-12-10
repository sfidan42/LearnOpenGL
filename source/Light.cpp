#include "Light.hpp"

Light::Light()
{
	glGenVertexArrays(1, &VAO);
}

Light::~Light()
{
	if (VAO)
		glDeleteVertexArrays(1, &VAO);
}

void Light::bind() const
{
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}
