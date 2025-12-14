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

struct Texture
{
	GLuint id;
	string type;
	string path;
};

struct Material
{
	vector<Texture> textures;
};

class Mesh
{
public:
	// mesh data
	vector<Vertex> vertices;
	vector<Index> indices;
	const Material* material;

	Mesh(const vector<Vertex>& vertices, const vector<unsigned int>& indices, const Material* material);

	void draw(const Shader& shader) const;

private:
	//  render data
	unsigned int VAO{}, VBO{}, EBO{};

	void setupMesh();
};
