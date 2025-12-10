#pragma once
#include "Texture.hpp"
#include "Vertex.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstddef>

struct InstanceData
{
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;

	static void bindAttributes(GLuint& idx)
	{
		// Model matrix occupies 4 attribute locations
		for(int i = 0; i < 4; ++i)
		{
			glEnableVertexAttribArray(idx);
			glVertexAttribPointer(idx, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
								  (void*)(offsetof(InstanceData, model) + sizeof(glm::vec4) * i));
			glVertexAttribDivisor(idx, 1); // Advance per instance
			idx++;
		}
	}

	void update(float deltaTime);

private: // send to GPU only
	glm::mat4 model;
};

class Model
{
public:
	Model();
	~Model();

	bool loadGeometry();
	bool loadTexture(const std::string& imagePath);
	bool instantiate(const InstanceData& data);

	bool bind() const;
	void update(float deltaTime);
	void draw() const;

private:
	// Instance Data
	std::vector<InstanceData> instances;

	// Model Data
	std::vector<Vertex> vertices;
	Texture texture{};
	GLuint VAO;
	GLuint VBO;
	GLuint instanceVBO;
};
