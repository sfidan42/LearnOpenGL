#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Shader.hpp"

using namespace std;
using namespace glm;

struct Vertex
{
	vec3 Position;
	vec3 Normal;
	vec2 TexCoords;

	static void vertexAttributes()
	{
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Normal)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, TexCoords)));
	}
};

using Index = uint32_t;

struct TransformComponent
{
	vec3 position;
	vec3 rotation;
	vec3 scale;
};

struct TextureComponent
{
	GLuint id;
	string type;
	string path;
};

struct MeshComponent
{
	void setup(const vector<Vertex>& vertices, const vector<Index>& indices, const vector<TextureComponent>& textures);
	void draw(const Shader& shader) const;
private:
	vector<Vertex> vertices;
	vector<Index> indices;
	vector<TextureComponent> textures;
	GLuint VAO{}, VBO{}, EBO{};
};
