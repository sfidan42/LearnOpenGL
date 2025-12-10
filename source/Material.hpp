#pragma once
#include "Shader.hpp"
#include "Texture.hpp"

struct Material
{
	explicit Material(const Texture& diffuseTex, const Texture& specularTex);
	~Material() = default;

	void bind() const;
	void send(const Shader& shader) const;

	float shininess{32.0f};
	glm::vec3 ambient{1.0f, 0.5f, 0.31f};

private:
	const Texture& diffuse;
	const Texture& specular;
};
