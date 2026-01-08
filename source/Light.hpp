#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include <entt/entity/registry.hpp>
#include "Components.hpp"
#include "Shadow.hpp"

using namespace glm;

class LightManager
{
public:
	LightManager(const Shader& mainShader, const Shader& skyShader);
	~LightManager();

	entt::entity createPointLight(const vec3& position, const vec3& color);
	entt::entity createSpotlight(const vec3& position, const vec3& direction, const vec3& color);
	entt::entity createDirLight(const vec3& direction, const vec3& color);

	PointLightComponent& getPointLight(entt::entity lightEntity);
	SpotlightComponent& getSpotlight(entt::entity lightEntity);
	DirLightComponent& getDirLight(entt::entity lightEntity);

	void updatePointLight(entt::entity lightEntity);
	void updateSpotlight(entt::entity lightEntity);
	void updateDirLight(entt::entity lightEntity);

	void deletePointLight(entt::entity lightEntity);
	void deleteSpotlight(entt::entity lightEntity);
	void deleteDirLight(entt::entity lightEntity);

	// Access internal registry for shadow rendering
	[[nodiscard]] entt::registry& getLightRegistry() { return lightRegistry; }
	[[nodiscard]] ShadowManager& getShadowManager() { return shadowManager; }

private:
	entt::registry lightRegistry;
	ShadowManager shadowManager;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO = 0;
	GLuint sunLightSSBO = 0;

	const Shader& cachedMainShader;
	const Shader& cachedSkyShader;

	void recalcSpotlightMatrix(entt::entity entityEntity);
	void recalcDirLightMatrix(entt::entity entityEntity);

	void syncPointLight(entt::entity lightEntity);
	void syncSpotlight(entt::entity lightEntity);
	void syncDirLight(entt::entity lightEntity);

	void syncPointLights();
	void syncSpotlights();
	void syncDirLights();
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
