#pragma once
#include "Texture.hpp"
#include "Vertex.hpp"
#include "Instance.hpp"

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
	std::vector<Instance> instances;
	std::vector<Vertex> vertices;
	Texture texture{};
	GLuint VAO;
	GLuint VBO;
};
