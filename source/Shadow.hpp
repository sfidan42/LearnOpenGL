#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <entt/entity/registry.hpp>

#include "Components.hpp"

using namespace glm;

// ==================== ShadowManager ====================
// Unified manager for all shadow map types using ECS components
class ShadowManager
{
public:
	explicit ShadowManager(entt::registry& registry);
	~ShadowManager() = default;

	// Non-copyable, non-movable
	ShadowManager(const ShadowManager&) = delete;
	ShadowManager& operator=(const ShadowManager&) = delete;
	ShadowManager(ShadowManager&&) = delete;
	ShadowManager& operator=(ShadowManager&&) = delete;

	// Point light shadow maps (cube map)
	GLuint64 createPointShadowMap(entt::entity lightEntity, uint32_t size = 1024);
	void destroyPointShadowMap(entt::entity lightEntity);
	void bindPointShadowForWriting(entt::entity lightEntity) const;
	void bindPointShadowForReading(entt::entity lightEntity, GLenum textureUnit) const;

	// Spotlight shadow maps (2D perspective)
	GLuint64 createSpotShadowMap(entt::entity lightEntity, uint32_t width = 1024, uint32_t height = 1024);
	void destroySpotShadowMap(entt::entity lightEntity);
	void bindSpotShadowForWriting(entt::entity lightEntity) const;
	void bindSpotShadowForReading(entt::entity lightEntity, GLenum textureUnit) const;

	// Directional light shadow maps (2D orthographic)
	GLuint64 createDirShadowMap(entt::entity lightEntity, uint32_t width = 4096, uint32_t height = 4096);
	void destroyDirShadowMap(entt::entity lightEntity);
	void bindDirShadowForWriting(entt::entity lightEntity) const;
	void bindDirShadowForReading(entt::entity lightEntity, GLenum textureUnit) const;

	// Utility functions for light space matrices
	static std::array<mat4, 6> getPointLightViewMatrices(const vec3& lightPos);
	static mat4 getPointLightProjection(float nearPlane = 0.1f, float farPlane = 50.0f);
	static mat4 getSpotLightSpaceMatrix(const vec3& position, const vec3& direction,
										float outerCutOff, float nearPlane = 0.1f, float farPlane = 50.0f);
	static mat4 getDirLightSpaceMatrix(const vec3& lightDir, float orthoSize = 20.0f,
									   float nearPlane = 1.0f, float farPlane = 50.0f);

	static constexpr float POINT_LIGHT_FAR_PLANE = 50.0f;

private:
	entt::registry& shadowRegistry;

	// Internal creation helpers
	static void setupPointShadowTexture(PointShadowMapComponent& comp);
	static void setupSpotShadowTexture(SpotShadowMapComponent& comp);
	static void setupDirShadowTexture(DirShadowMapComponent& comp);
};
