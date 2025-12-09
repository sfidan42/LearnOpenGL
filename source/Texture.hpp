#pragma once
#include <string>
#include <glad/glad.h>
#include <glm/vec2.hpp>

class Texture
{
public:
	Texture() = default;
	~Texture();

	bool load(const std::string& imagePath);

	void bind(unsigned int slot = 0) const;
	void unbind() const;

private:
	GLuint textureID ;
	glm::ivec2 size;
	int nrChannels;
};
