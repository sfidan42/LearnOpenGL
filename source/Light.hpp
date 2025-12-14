#pragma once
#include <glm/vec3.hpp>
#include "Shader.hpp"

using namespace glm;

struct DirLight
{
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight
{
	vec3 position;
	float constant;
	float linear;
	float quadratic;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct SpotLight
{
	vec3 position;
	vec3 direction;
	float cutOff;
	float outerCutOff;
	float constant;
	float linear;
	float quadratic;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

class LightManager
{
public:
	LightManager();
	~LightManager() = default;

	DirLight dirLight{};
	std::vector<PointLight> pointLights;
	std::vector<SpotLight> spotLights;

	void send(const Shader& shader) const;
};
