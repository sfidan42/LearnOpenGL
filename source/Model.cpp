#include "Model.hpp"
#include <glm/vec3.hpp>

Model::Model()
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
}

Model::~Model()
{
	if (VAO) glDeleteVertexArrays(1, &VAO);
	if (VBO) glDeleteBuffers(1, &VBO);
}

bool Model::loadGeometry()
{
	std::vector<glm::vec3> positions = {
		{-0.5f, -0.5f, 0.0f}, // bottom left
		{0.5f, -0.5f, 0.0f}, // bottom right
		{0.5f, 0.5f, 0.0f}, // top right
		{0.5f, 0.5f, 0.0f}, // top right
		{-0.5f, 0.5f, 0.0f}, // top left
		{-0.5f, -0.5f, 0.0f}, // bottom left
	};
	std::vector<glm::vec2> texCoords = {
		{0.0f, 0.0f}, // bottom left
		{1.0f, 0.0f}, // bottom right
		{1.0f, 1.0f}, // top right
		{1.0f, 1.0f}, // top right
		{0.0f, 1.0f}, // top left
		{0.0f, 0.0f}, // bottom left
	};

	uint32_t n = std::min<uint32_t>(texCoords.size(), positions.size());
	vertices.resize(n);
	for(uint32_t i = 0; i < n; i++)
	{
		vertices[i].position = positions[i];
		vertices[i].texCoord = texCoords[i];
	}
	return true;
}

bool Model::loadTexture(const std::string& imagePath)
{
	return texture.load(imagePath);
}

bool Model::bind()
{
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	Vertex::bindAttributes(); // <-- Bind attributes while VAO/VBO are bound
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	return true;
}

void Model::draw()
{
	texture.bind(0);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
