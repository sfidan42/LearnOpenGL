#include "Light.hpp"
#include <GLFW/glfw3.h>
#include <cmath>

Light::Light()
{
	glGenVertexArrays(1, &VAO);
}

Light::~Light()
{
	if(VAO)
		glDeleteVertexArrays(1, &VAO);
}

void Light::bind() const
{
	glBindVertexArray(VAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void Light::send(const Shader& shader) const
{
	shader.set3f("light.position", position.r, position.g, position.b);
	shader.set3f("light.ambient", ambient.r, ambient.g, ambient.b);
	shader.set3f("light.diffuse", diffuse.r, diffuse.g, ambient.b);
	shader.set3f("light.specular", specular.r, specular.g, specular.b);
}
