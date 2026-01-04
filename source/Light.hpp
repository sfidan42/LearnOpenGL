#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include <entt/entity/registry.hpp>
#include "Components.hpp"

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

class LightManager
{
public:
	LightManager(const Shader& mainShader, const Shader& skyShader);
	~LightManager();

	void update(float deltaTime, const Shader& mainShader, const Shader& skyShader);

	PointLight createPointLight(
		const vec3& position,
		const vec3& color
	);

	SpotLight createSpotLight(
		const vec3& position,
		const vec3& direction,
		const vec3& color
	);

	void syncPointLight(const PointLight& light);
	void syncSpotLight(const SpotLight& light);

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
