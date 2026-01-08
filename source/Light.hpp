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

class LightManager
{
public:
	LightManager(const Shader& mainShader, const Shader& skyShader);
	~LightManager();

	entt::entity createPointLight(
		const vec3& position,
		const vec3& color
	);

	entt::entity createSpotlight(
		const vec3& position,
		const vec3& direction,
		const vec3& color
	);

	entt::entity createDirLight(
		const vec3& direction,
		const vec3& color
	);

	PointLightComponent& getPointLight(entt::entity entity);
	SpotlightComponent& getSpotlight(entt::entity entity);
	DirLightComponent& getDirLight(entt::entity entity);

	void updatePointLight(entt::entity lightEntity);
	void updateSpotlight(entt::entity lightEntity);
	void updateDirLight(entt::entity lightEntity);

	void deletePointLight(entt::entity lightEntity);
	void deleteSpotlight(entt::entity lightEntity);
	void deleteDirLight(entt::entity lightEntity);

	// Access internal registry for shadow rendering
	[[nodiscard]] entt::registry& getLightRegistry() { return lightRegistry; }

private:
	entt::registry lightRegistry;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO = 0;
	GLuint sunLightSSBO = 0;

	const Shader& cachedMainShader;
	const Shader& cachedSkyShader;

	void recalcSpotlightMatrix(entt::entity entity);
	void recalcDirLightMatrix(entt::entity entity);

	void syncPointLights();
	void syncSpotlights();
	void syncDirLights();

	GLuint64 createPointLightShadowMap(entt::entity entity);
	GLuint64 createSpotlightShadowMap(entt::entity entity);
	GLuint64 createDirLightShadowMap(entt::entity entity);
	void destroyPointLightShadowMap(entt::entity entity);
	void destroySpotlightShadowMap(entt::entity entity);
	void destroyDirLightShadowMap(entt::entity entity);
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
