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

// Shadow map for a single point light (cubemap for omnidirectional shadows)
class PointLightShadowMap
{
public:
	explicit PointLightShadowMap(unsigned int size = 1024);
	~PointLightShadowMap();

	// Non-copyable
	PointLightShadowMap(const PointLightShadowMap&) = delete;
	PointLightShadowMap& operator=(const PointLightShadowMap&) = delete;

	// Move semantics
	PointLightShadowMap(PointLightShadowMap&& other) noexcept;
	PointLightShadowMap& operator=(PointLightShadowMap&& other) noexcept;

	void bindForWriting() const;
	void bindForReading(GLenum textureUnit) const;

	// Get the 6 view matrices for rendering to each cubemap face
	static std::array<mat4, 6> getLightViewMatrices(const vec3& lightPos);

	// Get projection matrix for point light (90 degree FOV)
	static mat4 getLightProjectionMatrix(float nearPlane = 0.1f, float farPlane = 50.0f);

	[[nodiscard]] GLuint getDepthCubemap() const { return depthCubemap; }
	[[nodiscard]] unsigned int getSize() const { return shadowSize; }

	static constexpr float FAR_PLANE = 50.0f;

private:
	GLuint depthMapFBO = 0;
	GLuint depthCubemap = 0;
	unsigned int shadowSize;
};

// Shadow map for a single spotlight (perspective 2D depth map)
class SpotlightShadowMap
{
public:
	explicit SpotlightShadowMap(unsigned int width = 1024, unsigned int height = 1024);
	~SpotlightShadowMap();

	// Non-copyable
	SpotlightShadowMap(const SpotlightShadowMap&) = delete;
	SpotlightShadowMap& operator=(const SpotlightShadowMap&) = delete;

	// Move semantics
	SpotlightShadowMap(SpotlightShadowMap&& other) noexcept;
	SpotlightShadowMap& operator=(SpotlightShadowMap&& other) noexcept;

	void bindForWriting() const;
	void bindForReading(GLenum textureUnit) const;

	// Calculate light space matrix for spotlight
	[[nodiscard]] mat4 getLightSpaceMatrix(const vec3& position, const vec3& direction,
										   float outerCutOff, float nearPlane = 0.1f, float farPlane = 50.0f) const;

	[[nodiscard]] GLuint getDepthTexture() const { return depthTexture; }
	[[nodiscard]] unsigned int getWidth() const { return shadowWidth; }
	[[nodiscard]] unsigned int getHeight() const { return shadowHeight; }

private:
	GLuint depthMapFBO = 0;
	GLuint depthTexture = 0;
	unsigned int shadowWidth;
	unsigned int shadowHeight;
};

class DirLightShadowMap
{
public:
	explicit DirLightShadowMap(unsigned int width = 2048, unsigned int height = 2048);
	~DirLightShadowMap();

	// Non-copyable
	DirLightShadowMap(const DirLightShadowMap&) = delete;
	DirLightShadowMap& operator=(const DirLightShadowMap&) = delete;

	// Bind framebuffer for shadow pass
	void bindForWriting() const;

	// Bind shadow map texture for reading in main shader
	void bindForReading(GLenum textureUnit) const;

	// Get light space matrix for directional light
	static mat4 getLightSpaceMatrix(const vec3& lightDir, float orthoSize = 20.0f, float nearPlane = 1.0f,
									float farPlane = 50.0f);

	[[nodiscard]] unsigned int getWidth() const { return shadowWidth; }
	[[nodiscard]] unsigned int getHeight() const { return shadowHeight; }
	[[nodiscard]] GLuint getDepthTexture() const { return depthTexture; }

private:
	GLuint depthMapFBO = 0;
	GLuint depthTexture = 0;
	uint32_t shadowWidth;
	uint32_t shadowHeight;
};
