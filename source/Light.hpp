#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include "entt/entity/registry.hpp"

using namespace glm;

struct DirLightGPU
{
	vec3 direction;
	float _pad0;
	vec3 ambient;
	float _pad1;
	vec3 diffuse;
	float _pad2;
	vec3 specular;
	float _pad3;
};

struct PointLightGPU
{
	vec3 position;
	float constant;    // Row 1: 16 bytes
	vec3 ambient;
	float linear;      // Row 2: 16 bytes
	vec3 diffuse;
	float quadratic;   // Row 3: 16 bytes
	vec3 specular;
	float _pad;        // Row 4: 16 bytes
}; // Total 64 bytes

struct SpotLightGPU
{
	vec3 position;
	float cutOff;      // Row 1
	vec3 direction;
	float outerCutOff; // Row 2
	vec3 ambient;
	float constant;    // Row 3
	vec3 diffuse;
	float linear;      // Row 4
	vec3 specular;
	float quadratic;   // Row 5
}; // Total 80 bytes

class LightManager
{
public:
	LightManager(const Shader& mainShader, const Shader& skyShader);
	~LightManager();

	void update(float deltaTime, const Shader& mainShader, const Shader& skyShader);

	PointLightGPU& createPointLight(
		const vec3& position,
		const vec3& color
	);

	SpotLightGPU& createSpotLight(
		const vec3& position,
		const vec3& direction,
		const vec3& color
	);

private:
	DirLightGPU sunLight{};

	entt::registry lightRegistry;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO = 0;

	float timeOfDay = 0.0f;

	const Shader& cachedMainShader;

	void setupLightTracking();
	void onLightUpdate(entt::registry& registry, entt::entity entity) const;
	void sendPointLights(const Shader& mainShader) const;
	void sendSpotLights(const Shader& mainShader) const;
	void sendSunLight(const Shader& mainShader, const Shader& skyShader) const;
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
