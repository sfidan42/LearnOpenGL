#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

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

// Shadow map for a single spot light (perspective 2D depth map)
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

	// Calculate light space matrix for spot light
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
