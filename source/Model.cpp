#include "Model.hpp"
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include <glm/ext/matrix_transform.hpp>

Model::Model()
    : VAO(0), VBO(0), instanceVBO(0)
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &instanceVBO); // Generate instance VBO
}

Model::~Model()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
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

bool Model::bind() const
{
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    Vertex::bindAttributes(); // <-- Bind attributes while VAO/VBO are bound

    // Setup instance attribute (mat4 modelMatrix)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // No data yet
    std::size_t vec4Size = sizeof(glm::vec4);
    GLsizei stride = sizeof(InstanceData);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, stride, (void*)(i * vec4Size));
        glVertexAttribDivisor(3 + i, 1);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return true;
}

void Model::drawInstanced(const std::vector<InstanceData>& instances) const
{
    texture.bind(0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)), instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(instances.size()));
    glBindVertexArray(0);
}

void Model::draw() const
{
    texture.bind(0);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
