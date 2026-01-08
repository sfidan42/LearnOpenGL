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

	// Handle case where direction is parallel to the default up vector
	vec3 normalizedDir = normalize(direction);
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	if (abs(dot(normalizedDir, up)) > 0.99f)
		up = vec3(1.0f, 0.0f, 0.0f); // Use X-axis as up when light points straight up/down

	mat4 view = lookAt(position, position + normalizedDir, up);

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
