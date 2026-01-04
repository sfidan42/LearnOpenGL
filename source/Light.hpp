#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include "entt/entity/registry.hpp"

using namespace glm;

struct alignas(16) DirLightGPU
{
	vec3 direction; float _pad0;
	vec3 ambient;   float _pad1;
	vec3 diffuse;   float _pad2;
	vec3 specular;  float _pad3;
};

struct alignas(16) PointLightGPU
{
	vec3 position; float constant;
	vec3 ambient;  float linear;
	vec3 diffuse;  float quadratic;
	vec3 specular; float _pad;
};

struct alignas(16) SpotLightGPU
{
	vec3 position;  float cutOff;
	vec3 direction; float outerCutOff;
	vec3 ambient;   float constant;
	vec3 diffuse;   float linear;
	vec3 specular;  float quadratic;
};

class LightManager
{
public:
	LightManager();
	~LightManager();

	void update(float deltaTime);
	void send(const Shader& mainShader, const Shader& skyShader) const;

private:
	DirLightGPU sunLight{};

	vector<PointLightGPU> pointLights;
	vector<SpotLightGPU> spotLights;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO  = 0;

	float timeOfDay = 0.0f;

	void updateSSBOs() const;
};
