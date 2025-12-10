#pragma once
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static void bindAttributes(GLuint& idx)
	{
		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(idx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		idx++;
		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(idx, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		idx++;
		glEnableVertexAttribArray(idx);
		glVertexAttribPointer(idx, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
		idx++;
	}
};
