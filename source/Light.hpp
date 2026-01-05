#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include <entt/entity/registry.hpp>
#include "Components.hpp"

using namespace glm;

struct PointLight
{
	PointLight(PointLightComponent& component, entt::entity entity);
	PointLightComponent& lightData;

private:
	entt::entity lightEntity;
	friend class LightManager;
};

struct SpotLight
{
	SpotLight(SpotLightComponent& component, entt::entity entity);
	SpotLightComponent& lightData;

private:
	entt::entity lightEntity;
	friend class LightManager;
};

class LightManager
{
public:
	LightManager(const Shader& mainShader, const Shader& skyShader);
	~LightManager();

	void update(float deltaTime);

	PointLight createPointLight(
		const vec3& position,
		const vec3& color
	);

	SpotLight createSpotLight(
		const vec3& position,
		const vec3& direction,
		const vec3& color
	);

	void updatePointLight(const PointLight& light);
	void updateSpotLight(const SpotLight& light);

	void deletePointLight(const PointLight& light);
	void deleteSpotLight(const SpotLight& light);

	[[nodiscard]] vec3 getSunDirection() const { return sunLight.direction; }

	// Access light data for shadow rendering
	[[nodiscard]] std::vector<PointLightComponent> getPointLights() const;
	[[nodiscard]] std::vector<SpotLightComponent> getSpotLights() const;

private:
	DirLightComponent sunLight{};

	entt::registry lightRegistry;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO = 0;

	float timeOfDay = 0.0f;

	const Shader& cachedMainShader;
	const Shader& cachedSkyShader;

	void setupLightTracking();
	void syncPointLights(entt::registry& registry, entt::entity entity);
	void syncSpotLights(entt::registry& registry, entt::entity entity);
	void syncSunLight() const;
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
