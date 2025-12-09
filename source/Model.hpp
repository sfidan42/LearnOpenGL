#pragma once
#include "Texture.hpp"
#include "Vertex.hpp"

class Model
{
public:
	Model();
	~Model();

	bool loadGeometry();
	bool loadTexture(const std::string& imagePath);

	bool bind();
	void draw();

private:
	std::vector<Vertex> vertices;
	Texture texture{};
	GLuint VAO;
	GLuint VBO;
};
