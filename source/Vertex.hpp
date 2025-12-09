#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex
{
	glm::vec3 position;

	static void bindAttributes()
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	}
};
