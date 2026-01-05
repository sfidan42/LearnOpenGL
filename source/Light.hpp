#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include <entt/entity/registry.hpp>
#include "Components.hpp"
#include "ShadowMap.hpp"
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

	// Get shadow maps for rendering
	[[nodiscard]] std::unordered_map<entt::entity, PointLightShadowMap*> getPointLightShadowMaps();
	[[nodiscard]] std::unordered_map<entt::entity, SpotLightShadowMap*> getSpotLightShadowMaps();

	// Access internal registry for shadow rendering
	[[nodiscard]] entt::registry& getLightRegistry() { return lightRegistry; }

	// Update light space matrices (call before syncing)
	void updateSpotLightMatrices();

private:
	DirLightComponent sunLight{};

	entt::registry lightRegistry;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO = 0;


	// Per-light shadow maps with bindless handles
	std::unordered_map<entt::entity, std::unique_ptr<PointLightShadowMap>> pointLightShadowMaps;
	std::unordered_map<entt::entity, std::unique_ptr<SpotLightShadowMap>> spotLightShadowMaps;
	std::unordered_map<entt::entity, GLuint64> pointLightShadowHandles;
	std::unordered_map<entt::entity, GLuint64> spotLightShadowHandles;

	float timeOfDay = 0.0f;

	const Shader& cachedMainShader;
	const Shader& cachedSkyShader;

	void setupLightTracking();
	void syncPointLights(entt::registry& registry, entt::entity entity);
	void syncSpotLights(entt::registry& registry, entt::entity entity);
	void syncSunLight() const;

	void createPointLightShadowMap(entt::entity entity);
	void createSpotLightShadowMap(entt::entity entity);
	void destroyPointLightShadowMap(entt::entity entity);
	void destroySpotLightShadowMap(entt::entity entity);
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
