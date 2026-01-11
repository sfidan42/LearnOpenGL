#pragma once
#include <glm/vec3.hpp>
#include <glad/glad.h>
#include "Shader.hpp"
#include <entt/entity/registry.hpp>
#include <functional>
#include "Components.hpp"

using namespace glm;

// Callback type for drawing models during shadow passes
using DrawModelsCallback = std::function<void(const Shader&)>;

class LightManager
{
public:
	LightManager(const Shader& mainShader, const Shader& skyShader, const Shader& shadowMapShader);
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

	// Shadow rendering - takes a callback to draw models
	void renderShadows(const DrawModelsCallback& drawModels);

private:
	entt::registry lightRegistry;

	GLuint pointLightSSBO = 0;
	GLuint spotLightSSBO = 0;
	GLuint sunLightSSBO = 0;

	const Shader& cachedMainShader;
	const Shader& cachedSkyShader;
	const Shader& cachedShadowMapShader;

	void recalcPointLightMatrices(entt::entity lightEntity);
	void recalcSpotlightMatrix(entt::entity entityEntity);
	void recalcDirLightMatrix(entt::entity entityEntity);

	void syncPointLight(entt::entity lightEntity);
	void syncSpotlight(entt::entity lightEntity);
	void syncDirLight(entt::entity lightEntity);

	void syncPointLights();
	void syncSpotlights();
	void syncDirLights();

	// ============ Shadows ============ //

	// Point light shadow maps (cube map)
	GLuint64 createPointShadowMap(entt::entity lightEntity, uint32_t size = 1024);
	void destroyPointShadowMap(entt::entity lightEntity);

	// Spotlight shadow maps (2D perspective)
	GLuint64 createSpotShadowMap(entt::entity lightEntity, uint32_t width = 1024, uint32_t height = 1024);
	void destroySpotShadowMap(entt::entity lightEntity);

	// Directional light shadow maps (2D orthographic)
	GLuint64 createDirShadowMap(entt::entity lightEntity, uint32_t width = 4096, uint32_t height = 4096);
	void destroyDirShadowMap(entt::entity lightEntity);

	// Utility functions for light space matrices
	static std::array<mat4, 6> getPointLightViewMatrices(const vec3& lightPos);
	static mat4 getPointLightProjection(float nearPlane = 0.1f, float farPlane = 50.0f);
	static mat4 getSpotLightSpaceMatrix(const vec3& position, const vec3& direction,
										float outerCutOff, float nearPlane = 0.1f, float farPlane = 50.0f);
	static mat4 getDirLightSpaceMatrix(const vec3& lightDir, float orthoSize = 20.0f,
									   float nearPlane = 1.0f, float farPlane = 50.0f);

	static constexpr float POINT_LIGHT_FAR_PLANE = 50.0f;

	// Shadow texture setup helpers
	static void setupPointShadowTexture(PointShadowMapComponent& comp);
	static void setupSpotShadowTexture(SpotShadowMapComponent& comp);
	static void setupDirShadowTexture(DirShadowMapComponent& comp);

	// Shadow rendering implementations
	void renderDirLightShadows(const DrawModelsCallback& drawModels);
	void renderPointLightShadows(const DrawModelsCallback& drawModels);
	void renderSpotlightShadows(const DrawModelsCallback& drawModels);
};

void setupLightTracking(LightManager& lManager, entt::registry& registry, const Shader& mainShader);
