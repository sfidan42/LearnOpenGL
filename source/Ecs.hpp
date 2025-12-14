#pragma once
#include "Mesh.hpp"

struct MeshComponent
{
	void setup(const vector<Vertex>& vertices, const vector<Index>& indices, const vector<Texture>& textures);
	void draw(const Shader& shader) const;
private:
	vector<Vertex> vertices;
	vector<Index> indices;
	vector<Texture> textures;
	GLuint VAO{}, VBO{}, EBO{};
};
