#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include <entt/entity/registry.hpp>
#include "Components.hpp"
#include "Shadow.hpp"
#include <memory>
#include <unordered_map>

using namespace glm;

struct PointLight
{
	PointLight(PointLightComponent& component, entt::entity entity);
	PointLightComponent& lightData;

private:
	entt::entity lightEntity;
	friend class LightManager;
};

struct Spotlight
{
	Spotlight(SpotlightComponent& component, entt::entity entity);
	SpotlightComponent& lightData;

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

	Spotlight createSpotlight(
		const vec3& position,
		const vec3& direction,
		const vec3& color
	);

	void updatePointLight(const PointLight& light);
	void updateSpotlight(const Spotlight& light);

	void deletePointLight(const PointLight& light);
	void deleteSpotlight(const Spotlight& light);

	[[nodiscard]] vec3 getSunDirection() const { return sunLight.direction; }

	// Access internal registry for shadow rendering
	[[nodiscard]] entt::registry& getLightRegistry() { return lightRegistry; }

	// Update light space matrices (call before syncing)
	void updateSpotlightMatrices();

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
	void syncSpotlights(entt::registry& registry, entt::entity entity);
	void syncSunLight() const;

	GLuint64 createPointLightShadowMap(entt::entity entity);
	GLuint64 createSpotlightShadowMap(entt::entity entity);
	void destroyPointLightShadowMap(entt::entity entity);
	void destroySpotlightShadowMap(entt::entity entity);
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
