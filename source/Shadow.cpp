#include "Shadow.hpp"
#include <iostream>
#include <array>

// ==================== ShadowManager ====================

ShadowManager::ShadowManager(entt::registry& registry)
	: shadowRegistry(registry)
{
}

// -------------------- Point Light Shadow Maps --------------------

void ShadowManager::setupPointShadowTexture(PointShadowMapComponent& comp)
{
	glGenFramebuffers(1, &comp.frameBuffer);
	glGenTextures(1, &comp.depthCubeMap);

	glBindTexture(GL_TEXTURE_CUBE_MAP, comp.depthCubeMap);
	for (unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
					 comp.shadowSize, comp.shadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindFramebuffer(GL_FRAMEBUFFER, comp.frameBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, comp.depthCubeMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Point light shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint64 ShadowManager::createPointShadowMap(entt::entity lightEntity, uint32_t size)
{
	auto& comp = shadowRegistry.emplace<PointShadowMapComponent>(lightEntity);
	comp.shadowSize = size;
	setupPointShadowTexture(comp);

	// Create bindless handle
	GLuint64 handle = glGetTextureHandleARB(comp.depthCubeMap);
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void ShadowManager::destroyPointShadowMap(entt::entity lightEntity)
{
	if (auto* comp = shadowRegistry.try_get<PointShadowMapComponent>(lightEntity))
	{
		GLuint64 handle = glGetTextureHandleARB(comp->depthCubeMap);
		if (glIsTextureHandleResidentARB(handle))
			glMakeTextureHandleNonResidentARB(handle);

		if (comp->depthCubeMap)
			glDeleteTextures(1, &comp->depthCubeMap);
		if (comp->frameBuffer)
			glDeleteFramebuffers(1, &comp->frameBuffer);

		shadowRegistry.remove<PointShadowMapComponent>(lightEntity);
	}
}

void ShadowManager::bindPointShadowForWriting(entt::entity lightEntity) const
{
	if (const auto* comp = shadowRegistry.try_get<PointShadowMapComponent>(lightEntity))
	{
		glViewport(0, 0, comp->shadowSize, comp->shadowSize);
		glBindFramebuffer(GL_FRAMEBUFFER, comp->frameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

void ShadowManager::bindPointShadowForReading(entt::entity lightEntity, GLenum textureUnit) const
{
	if (const auto* comp = shadowRegistry.try_get<PointShadowMapComponent>(lightEntity))
	{
		glActiveTexture(textureUnit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, comp->depthCubeMap);
	}
}

std::array<mat4, 6> ShadowManager::getPointLightViewMatrices(const vec3& lightPos)
{
	return {
		lookAt(lightPos, lightPos + vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),  // +X
		lookAt(lightPos, lightPos + vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)), // -X
		lookAt(lightPos, lightPos + vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),   // +Y
		lookAt(lightPos, lightPos + vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)), // -Y
		lookAt(lightPos, lightPos + vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),  // +Z
		lookAt(lightPos, lightPos + vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f))  // -Z
	};
}

mat4 ShadowManager::getPointLightProjection(float nearPlane, float farPlane)
{
	return perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
}

// -------------------- Spotlight Shadow Maps --------------------

void ShadowManager::setupSpotShadowTexture(SpotShadowMapComponent& comp)
{
	glGenFramebuffers(1, &comp.frameBuffer);
	glGenTextures(1, &comp.depthTexture);

	glBindTexture(GL_TEXTURE_2D, comp.depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, comp.shadowWidth, comp.shadowHeight,
				 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Enable hardware PCF
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glBindFramebuffer(GL_FRAMEBUFFER, comp.frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, comp.depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Spotlight shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint64 ShadowManager::createSpotShadowMap(entt::entity lightEntity, uint32_t width, uint32_t height)
{
	auto& comp = shadowRegistry.emplace<SpotShadowMapComponent>(lightEntity);
	comp.shadowWidth = width;
	comp.shadowHeight = height;
	setupSpotShadowTexture(comp);

	GLuint64 handle = glGetTextureHandleARB(comp.depthTexture);
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void ShadowManager::destroySpotShadowMap(entt::entity lightEntity)
{
	if (auto* comp = shadowRegistry.try_get<SpotShadowMapComponent>(lightEntity))
	{
		GLuint64 handle = glGetTextureHandleARB(comp->depthTexture);
		if (glIsTextureHandleResidentARB(handle))
			glMakeTextureHandleNonResidentARB(handle);

		if (comp->depthTexture)
			glDeleteTextures(1, &comp->depthTexture);
		if (comp->frameBuffer)
			glDeleteFramebuffers(1, &comp->frameBuffer);

		shadowRegistry.remove<SpotShadowMapComponent>(lightEntity);
	}
}

void ShadowManager::bindSpotShadowForWriting(entt::entity lightEntity) const
{
	if (const auto* comp = shadowRegistry.try_get<SpotShadowMapComponent>(lightEntity))
	{
		glViewport(0, 0, comp->shadowWidth, comp->shadowHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, comp->frameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

void ShadowManager::bindSpotShadowForReading(entt::entity lightEntity, GLenum textureUnit) const
{
	if (const auto* comp = shadowRegistry.try_get<SpotShadowMapComponent>(lightEntity))
	{
		glActiveTexture(textureUnit);
		glBindTexture(GL_TEXTURE_2D, comp->depthTexture);
	}
}

mat4 ShadowManager::getSpotLightSpaceMatrix(const vec3& position, const vec3& direction,
											float outerCutOff, float nearPlane, float farPlane)
{
	float fov = acos(outerCutOff) * 2.0f;
	fov = glm::clamp(fov, glm::radians(10.0f), glm::radians(170.0f));

	mat4 projection = perspective(fov, 1.0f, nearPlane, farPlane);
	mat4 view = lookAt(position, position + normalize(direction), vec3(0.0f, 1.0f, 0.0f));

	return projection * view;
}

// -------------------- Directional Light Shadow Maps --------------------

void ShadowManager::setupDirShadowTexture(DirShadowMapComponent& comp)
{
	glGenFramebuffers(1, &comp.frameBuffer);
	glGenTextures(1, &comp.depthTexture);

	glBindTexture(GL_TEXTURE_2D, comp.depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, comp.shadowWidth, comp.shadowHeight,
				 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Enable anisotropic filtering for better quality
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

	// Enable hardware PCF
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glBindFramebuffer(GL_FRAMEBUFFER, comp.frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, comp.depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Directional light shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint64 ShadowManager::createDirShadowMap(entt::entity lightEntity, uint32_t width, uint32_t height)
{
	auto& comp = shadowRegistry.emplace<DirShadowMapComponent>(lightEntity);
	comp.shadowWidth = width;
	comp.shadowHeight = height;
	setupDirShadowTexture(comp);

	GLuint64 handle = glGetTextureHandleARB(comp.depthTexture);
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void ShadowManager::destroyDirShadowMap(entt::entity lightEntity)
{
	if (auto* comp = shadowRegistry.try_get<DirShadowMapComponent>(lightEntity))
	{
		GLuint64 handle = glGetTextureHandleARB(comp->depthTexture);
		if (glIsTextureHandleResidentARB(handle))
			glMakeTextureHandleNonResidentARB(handle);

		if (comp->depthTexture)
			glDeleteTextures(1, &comp->depthTexture);
		if (comp->frameBuffer)
			glDeleteFramebuffers(1, &comp->frameBuffer);

		shadowRegistry.remove<DirShadowMapComponent>(lightEntity);
	}
}

void ShadowManager::bindDirShadowForWriting(entt::entity lightEntity) const
{
	if (const auto* comp = shadowRegistry.try_get<DirShadowMapComponent>(lightEntity))
	{
		glViewport(0, 0, comp->shadowWidth, comp->shadowHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, comp->frameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
}

void ShadowManager::bindDirShadowForReading(entt::entity lightEntity, GLenum textureUnit) const
{
	if (const auto* comp = shadowRegistry.try_get<DirShadowMapComponent>(lightEntity))
	{
		glActiveTexture(textureUnit);
		glBindTexture(GL_TEXTURE_2D, comp->depthTexture);
	}
}

mat4 ShadowManager::getDirLightSpaceMatrix(const vec3& lightDir, float orthoSize, float nearPlane, float farPlane)
{
	mat4 lightProjection = ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
	mat4 lightView = lookAt(-normalize(lightDir) * 25.0f, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
	return lightProjection * lightView;
}


// ==================== PointLightShadowMap ====================

PointLightShadowMap::PointLightShadowMap(unsigned int size)
: shadowSize(size)
{
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthCubemap);

	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
	for(unsigned int i = 0; i < 6; ++i)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
					 shadowSize, shadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// No hardware comparison for cubemaps - we do manual depth comparison in shader

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Point light shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PointLightShadowMap::~PointLightShadowMap()
{
	if(depthCubemap)
		glDeleteTextures(1, &depthCubemap);
	if(depthMapFBO)
		glDeleteFramebuffers(1, &depthMapFBO);
}

PointLightShadowMap::PointLightShadowMap(PointLightShadowMap&& other) noexcept
: depthMapFBO(other.depthMapFBO), depthCubemap(other.depthCubemap), shadowSize(other.shadowSize)
{
	other.depthMapFBO = 0;
	other.depthCubemap = 0;
}

PointLightShadowMap& PointLightShadowMap::operator=(PointLightShadowMap&& other) noexcept
{
	if(this != &other)
	{
		if(depthCubemap)
			glDeleteTextures(1, &depthCubemap);
		if(depthMapFBO)
			glDeleteFramebuffers(1, &depthMapFBO);

		depthMapFBO = other.depthMapFBO;
		depthCubemap = other.depthCubemap;
		shadowSize = other.shadowSize;

		other.depthMapFBO = 0;
		other.depthCubemap = 0;
	}
	return *this;
}

void PointLightShadowMap::bindForWriting() const
{
	glViewport(0, 0, shadowSize, shadowSize);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void PointLightShadowMap::bindForReading(GLenum textureUnit) const
{
	glActiveTexture(textureUnit);
	glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
}

std::array<mat4, 6> PointLightShadowMap::getLightViewMatrices(const vec3& lightPos)
{
	return {
		lookAt(lightPos, lightPos + vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)), // +X
		lookAt(lightPos, lightPos + vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)), // -X
		lookAt(lightPos, lightPos + vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)), // +Y
		lookAt(lightPos, lightPos + vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)), // -Y
		lookAt(lightPos, lightPos + vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)), // +Z
		lookAt(lightPos, lightPos + vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f)) // -Z
	};
}

mat4 PointLightShadowMap::getLightProjectionMatrix(float nearPlane, float farPlane)
{
	// 90 degree FOV for cubemap face, aspect ratio 1:1
	return perspective(glm::radians(90.0f), 1.0f, nearPlane, farPlane);
}


// ==================== SpotlightShadowMap ====================

SpotlightShadowMap::SpotlightShadowMap(unsigned int width, unsigned int height)
: shadowWidth(width), shadowHeight(height)
{
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthTexture);

	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
				 nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Enable hardware PCF
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Spot light shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

SpotlightShadowMap::~SpotlightShadowMap()
{
	if(depthTexture)
		glDeleteTextures(1, &depthTexture);
	if(depthMapFBO)
		glDeleteFramebuffers(1, &depthMapFBO);
}

SpotlightShadowMap::SpotlightShadowMap(SpotlightShadowMap&& other) noexcept
: depthMapFBO(other.depthMapFBO), depthTexture(other.depthTexture),
  shadowWidth(other.shadowWidth), shadowHeight(other.shadowHeight)
{
	other.depthMapFBO = 0;
	other.depthTexture = 0;
}

SpotlightShadowMap& SpotlightShadowMap::operator=(SpotlightShadowMap&& other) noexcept
{
	if(this != &other)
	{
		if(depthTexture)
			glDeleteTextures(1, &depthTexture);
		if(depthMapFBO)
			glDeleteFramebuffers(1, &depthMapFBO);

		depthMapFBO = other.depthMapFBO;
		depthTexture = other.depthTexture;
		shadowWidth = other.shadowWidth;
		shadowHeight = other.shadowHeight;

		other.depthMapFBO = 0;
		other.depthTexture = 0;
	}
	return *this;
}

void SpotlightShadowMap::bindForWriting() const
{
	glViewport(0, 0, shadowWidth, shadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void SpotlightShadowMap::bindForReading(GLenum textureUnit) const
{
	glActiveTexture(textureUnit);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
}

mat4 SpotlightShadowMap::getLightSpaceMatrix(const vec3& position, const vec3& direction,
											 const float outerCutOff, const float nearPlane, const float farPlane) const
{
	// FOV should be at least as wide as the outer cone angle (outerCutOff is cos of angle)
	float fov = acos(outerCutOff) * 2.0f;
	fov = glm::clamp(fov, glm::radians(30.0f), glm::radians(120.0f));

	float aspect = static_cast<float>(shadowWidth) / static_cast<float>(shadowHeight);
	mat4 lightProjection = perspective(fov, aspect, nearPlane, farPlane);

	// Calculate up vector that isn't parallel to direction
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	if(abs(dot(direction, up)) > 0.99f)
		up = vec3(1.0f, 0.0f, 0.0f);

	mat4 lightView = lookAt(position, position + direction, up);

	return lightProjection * lightView;
}

// ==================== ShadowMap (for directional light) ====================

DirLightShadowMap::DirLightShadowMap(const uint32_t width, const uint32_t height)
: shadowWidth(width), shadowHeight(height)
{
	// Create framebuffer for shadow map
	glGenFramebuffers(1, &depthMapFBO);

	// Create depth texture
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT,
				 nullptr);

	// Texture parameters for better quality
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Set border color to 1.0 (no shadow outside the shadow map)
	const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Enable anisotropic filtering if available for better quality at angles
	float maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);

	// Enable hardware PCF (Percentage Closer Filtering)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	// Attach depth texture to framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	// No color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// Check framebuffer completeness
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "ERROR: Shadow map framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

DirLightShadowMap::~DirLightShadowMap()
{
	if(depthTexture)
		glDeleteTextures(1, &depthTexture);
	if(depthMapFBO)
		glDeleteFramebuffers(1, &depthMapFBO);
}

void DirLightShadowMap::bindForWriting() const
{
	glViewport(0, 0, shadowWidth, shadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void DirLightShadowMap::bindForReading(GLenum textureUnit) const
{
	glActiveTexture(textureUnit);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
}

mat4 DirLightShadowMap::getLightSpaceMatrix(const vec3& lightDir, const float orthoSize, const float nearPlane,
									const float farPlane)
{
	// For directional light, we use orthographic projection
	const mat4 lightProjection = ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

	// Normalize the light direction
	const vec3 dir = normalize(lightDir);

	// Position light looking down along light direction, centered at scene
	// The light needs to be positioned "behind" relative to the light direction
	const vec3 sceneCenter = vec3(0.0f, 0.0f, 0.0f);
	const vec3 lightPos = sceneCenter - dir * 50.0f;  // Fixed distance, not dependent on farPlane

	// Calculate up vector that isn't parallel to light direction
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	if (abs(dot(dir, up)) > 0.99f)
		up = vec3(1.0f, 0.0f, 0.0f);  // Use X-axis as up when light is vertical

	const mat4 lightView = lookAt(lightPos, sceneCenter, up);

	return lightProjection * lightView;
}
