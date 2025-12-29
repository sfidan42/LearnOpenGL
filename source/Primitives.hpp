#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoords;

	static GLuint vertexAttributes()
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));
		return 3; // Next available attribute location
	}
};

using Index = uint32_t;
