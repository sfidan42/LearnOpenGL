#include "Model.hpp"
#include <glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <iostream>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/random.hpp>

void InstanceData::update(float deltaTime)
{
	rotation += glm::linearRand(glm::vec3(0.0f), glm::vec3(glm::two_pi<float>())) * 0.005f;

	model = glm::mat4(1.0f);
	model = glm::translate(model, translation);
	model = glm::rotate(model, rotation.x, glm::vec3(1,0,0));
	model = glm::rotate(model, rotation.y, glm::vec3(0,1,0));
	model = glm::rotate(model, rotation.z, glm::vec3(0,0,1));
	model = glm::scale(model, scale);
}

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
	float verts[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};
	constexpr size_t n = sizeof(verts) / (5 * sizeof(float));
	vertices.resize(n);
	for(uint32_t i = 0; i < n; i++)
	{
		const glm::vec3 position(verts[i * 5 + 0], verts[i * 5 + 1], verts[i * 5 + 2]);
		const glm::vec2 texCoord(verts[i * 5 + 3], verts[i * 5 + 4]);
		vertices[i].position = position;
		vertices[i].texCoord = texCoord;
	}
	std::cout << "Loaded " << vertices.size() << " vertices." << std::endl;
	return true;
}

bool Model::loadTexture(const std::string& imagePath)
{
	return texture.load(imagePath);
}

bool Model::instantiate(const InstanceData& data)
{
	if (glm::length(data.scale) <= 0.0f)
		return false; // Invalid scale
	instances.emplace_back(data);
	instances.back().update(0.0f);
	return true;
}

bool Model::bind() const
{
    GLuint idx = 0;

	glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
	Vertex::bindAttributes(idx); // <-- Bind attributes while VAO/VBO are bound

    // Setup instance attribute (mat4 modelMatrix)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // No data yet
    InstanceData::bindAttributes(idx);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

	return idx > 0;
}

void Model::update(float deltaTime)
{
	for(auto& instance : instances)
		instance.update(deltaTime);
}

void Model::draw() const
{
    texture.bind(0);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)), instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()), static_cast<GLsizei>(instances.size()));
    glBindVertexArray(0);
}
