#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex
{
	glm::vec3 position;
	glm::vec2 texCoord;

	static void bindAttributes()
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
	}
};
