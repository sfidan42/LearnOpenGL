#include "Light.hpp"
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

LightManager::LightManager(const Shader& mainShader, const Shader& skyShader, const Shader& shadowMapShader, const Shader& shadowPointShader)
: cachedMainShader(mainShader), cachedSkyShader(skyShader), cachedShadowMapShader(shadowMapShader), cachedShadowPointShader(shadowPointShader)
{
	// Create SSBOs for dynamic lights
	glGenBuffers(1, &pointLightSSBO);
	glGenBuffers(1, &spotLightSSBO);
	glGenBuffers(1, &sunLightSSBO);
}

LightManager::~LightManager()
{
	if(pointLightSSBO)
		glDeleteBuffers(1, &pointLightSSBO);
	if(spotLightSSBO)
		glDeleteBuffers(1, &spotLightSSBO);
	if(sunLightSSBO)
		glDeleteBuffers(1, &sunLightSSBO);
}

entt::entity LightManager::createPointLight(const vec3& position, const vec3& color)
{
	PointLightComponent lightComp{};
	lightComp.position = position;
	lightComp.constant = 1.0f;
	lightComp.ambient = color * 0.1f;
	lightComp.linear = 0.09f;
	lightComp.diffuse = color;
	lightComp.quadratic = 0.032f;
	lightComp.specular = color;
	lightComp.farPlane = POINT_LIGHT_FAR_PLANE;
	lightComp.cubeMapHandle = 0; // Will be set after shadow map creation

	const entt::entity lightEnt = lightRegistry.create();
	auto& comp = lightRegistry.emplace<PointLightComponent>(lightEnt, lightComp);

	// Create shadow map for this light using ShadowManager
	comp.cubeMapHandle = createPointShadowMap(lightEnt, 512);

	// Calculate shadow matrices AFTER shadow map is created
	recalcPointLightMatrices(lightEnt);
	syncPointLights();

	return lightEnt;
}

entt::entity LightManager::createSpotlight(const vec3& position, const vec3& direction, const vec3& color)
{
	SpotlightComponent lightComp{};
	lightComp.position = position;
	lightComp.cutOff = cos(radians(12.5f));
	lightComp.direction = direction;
	lightComp.outerCutOff = cos(radians(17.5f));
	lightComp.ambient = color * 0.1f;
	lightComp.constant = 1.0f;
	lightComp.diffuse = color;
	lightComp.linear = 0.09f;
	lightComp.specular = color;
	lightComp.quadratic = 0.032f;
	lightComp.lightSpaceMatrix = mat4(1.0f); // Will be updated below
	lightComp.shadowMapHandle = 0; // Will be set after shadow map creation

	const entt::entity lightEnt = lightRegistry.create();
	auto& comp = lightRegistry.emplace<SpotlightComponent>(lightEnt, lightComp);

	// Create shadow map for this light using ShadowManager
	comp.shadowMapHandle = createSpotShadowMap(lightEnt, 2048, 2048);

	// Calculate light space matrix AFTER shadow map is created
	recalcSpotlightMatrix(lightEnt);
	syncSpotlights();

	return lightEnt;
}

entt::entity LightManager::createDirLight(const vec3& direction, const vec3& color)
{
	DirLightComponent lightComp{};
	lightComp.direction = direction;
	lightComp.ambient = color * 0.1f;
	lightComp.diffuse = color;
	lightComp.specular = color;
	lightComp.lightSpaceMatrix = mat4(1.0f); // Will be updated below

	const entt::entity lightEnt = lightRegistry.create();
	auto& comp = lightRegistry.emplace<DirLightComponent>(lightEnt, lightComp);

	// Create shadow map for this light using ShadowManager
	comp.shadowMapHandle = createDirShadowMap(lightEnt, 4096, 4096);

	// Calculate light space matrix AFTER shadow map is created
	recalcDirLightMatrix(lightEnt);
	syncDirLights();

	return lightEnt;
}

PointLightComponent& LightManager::getPointLight(const entt::entity lightEntity)
{
	assert(lightRegistry.all_of<PointLightComponent>(lightEntity) && "Entity does not have PointLightComponent");
	return lightRegistry.get<PointLightComponent>(lightEntity);
}

SpotlightComponent& LightManager::getSpotlight(const entt::entity lightEntity)
{
	assert(lightRegistry.all_of<SpotlightComponent>(lightEntity) && "Entity does not have SpotlightComponent");
	return lightRegistry.get<SpotlightComponent>(lightEntity);
}

DirLightComponent& LightManager::getDirLight(const entt::entity lightEntity)
{
	assert(lightRegistry.all_of<DirLightComponent>(lightEntity) && "Entity does not have DirLightComponent");
	return lightRegistry.get<DirLightComponent>(lightEntity);
}

void LightManager::updatePointLight(const entt::entity lightEntity)
{
	recalcPointLightMatrices(lightEntity);
	syncPointLight(lightEntity);
}

void LightManager::updateSpotlight(const entt::entity lightEntity)
{
	recalcSpotlightMatrix(lightEntity);
	syncSpotlight(lightEntity);
}

void LightManager::updateDirLight(const entt::entity lightEntity)
{
	recalcDirLightMatrix(lightEntity);
	syncDirLight(lightEntity);
}

void LightManager::deletePointLight(const entt::entity lightEntity)
{
	destroyPointShadowMap(lightEntity);
	lightRegistry.destroy(lightEntity);
	syncPointLights();
}

void LightManager::deleteSpotlight(const entt::entity lightEntity)
{
	destroySpotShadowMap(lightEntity);
	lightRegistry.destroy(lightEntity);
	syncSpotlights();
}

void LightManager::deleteDirLight(const entt::entity lightEntity)
{
	destroyDirShadowMap(lightEntity);
	lightRegistry.destroy(lightEntity);
	syncDirLights();
}

void LightManager::renderShadows(const DrawModelsCallback& drawModels)
{
	renderDirLightShadows(drawModels);
	renderPointLightShadows(drawModels);
	renderSpotlightShadows(drawModels);
}

void LightManager::recalcPointLightMatrices(const entt::entity lightEntity)
{
	auto* pLight = lightRegistry.try_get<PointLightComponent>(lightEntity);
	if(!pLight)
		return;

	const float nearPlane = 0.1f;
	const float farPlane = pLight->farPlane;
	const mat4 projection = getPointLightProjection(nearPlane, farPlane);
	const auto viewMatrices = getPointLightViewMatrices(pLight->position);

	for(int face = 0; face < 6; ++face)
		pLight->shadowMatrices[face] = projection * viewMatrices[face];
}

void LightManager::recalcSpotlightMatrix(const entt::entity lightEntity)
{
	auto* sLight = lightRegistry.try_get<SpotlightComponent>(lightEntity);
	if(!sLight)
		return;

	sLight->lightSpaceMatrix = getSpotLightSpaceMatrix(
		sLight->position,
		normalize(sLight->direction),
		sLight->outerCutOff, 0.1f, 50.0f
	);
}

void LightManager::recalcDirLightMatrix(const entt::entity lightEntity)
{
	auto* dirLight = lightRegistry.try_get<DirLightComponent>(lightEntity);
	if(!dirLight)
		return;

	dirLight->lightSpaceMatrix = getDirLightSpaceMatrix(
		dirLight->direction,
		50.0f, // ortho size - covers -50 to +50 on X/Y in light space
		0.1f, // near plane
		150.0f // far plane
	);
}

void LightManager::syncPointLight(const entt::entity lightEntity)
{
	const PointLightComponent& pointLight = getPointLight(lightEntity);
	const GLuint index = static_cast<GLuint>(lightRegistry.view<PointLightComponent>()->index(lightEntity));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLightSSBO);
	glBufferSubData(
		GL_SHADER_STORAGE_BUFFER,
		index * sizeof(PointLightComponent),
		sizeof(PointLightComponent),
		&pointLight
	);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightManager::syncSpotlight(const entt::entity lightEntity)
{
	const SpotlightComponent& spotLight = getSpotlight(lightEntity);
	const GLuint index = static_cast<GLuint>(lightRegistry.view<SpotlightComponent>()->index(lightEntity));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotLightSSBO);
	glBufferSubData(
		GL_SHADER_STORAGE_BUFFER,
		index * sizeof(SpotlightComponent),
		sizeof(SpotlightComponent),
		&spotLight
	);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightManager::syncDirLight(const entt::entity lightEntity)
{
	const DirLightComponent& dirLight = getDirLight(lightEntity);
	const GLuint index = static_cast<GLuint>(lightRegistry.view<DirLightComponent>()->index(lightEntity));
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sunLightSSBO);
	glBufferSubData(
		GL_SHADER_STORAGE_BUFFER,
		index * sizeof(DirLightComponent),
		sizeof(DirLightComponent),
		&dirLight
	);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightManager::syncPointLights()
{
	const uint32_t pLightsCount = lightRegistry.view<PointLightComponent>().size();

	cachedMainShader.use();
	cachedMainShader.setInt("u_numPointLights", pLightsCount);

	vector<PointLightComponent> pointLights;
	pointLights.reserve(pLightsCount);
	const auto& pLightView = lightRegistry.view<PointLightComponent>();
	pLightView.each([&](const PointLightComponent& light) { pointLights.push_back(light); });

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		pLightsCount * sizeof(PointLightComponent),
		pointLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(SSBOBindingPoint::PointLights), pointLightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightManager::syncSpotlights()
{
	const uint32_t sLightsCount = lightRegistry.view<SpotlightComponent>().size();

	cachedMainShader.use();
	cachedMainShader.setInt("u_numSpotLights", sLightsCount);

	vector<SpotlightComponent> spotLights;
	spotLights.reserve(sLightsCount);
	const auto& sLightView = lightRegistry.view<SpotlightComponent>();
	sLightView.each([&](const SpotlightComponent& light) { spotLights.push_back(light); });

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		sLightsCount * sizeof(SpotlightComponent),
		spotLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(SSBOBindingPoint::Spotlights), spotLightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightManager::syncDirLights()
{
	const uint32_t dLightsCount = lightRegistry.view<DirLightComponent>().size();

	cachedMainShader.use();
	cachedMainShader.setInt("u_numDirLights", dLightsCount);

	vector<DirLightComponent> dirLights;
	dirLights.reserve(dLightsCount);
	const auto& dLightView = lightRegistry.view<DirLightComponent>();
	dLightView.each([&](const DirLightComponent& light) { dirLights.push_back(light); });

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sunLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		dLightsCount * sizeof(DirLightComponent),
		dirLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(SSBOBindingPoint::DirLights), sunLightSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	cachedSkyShader.use();
	cachedSkyShader.setInt("u_numDirLights", dLightsCount);
}

GLuint64 LightManager::createPointShadowMap(entt::entity lightEntity, uint32_t size)
{
	auto& comp = lightRegistry.emplace<PointShadowMapComponent>(lightEntity);
	comp.shadowSize = size;
	setupPointShadowTexture(comp);

	// Create bindless handle
	const GLuint64 handle = glGetTextureHandleARB(comp.depthCubeMap);
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void LightManager::destroyPointShadowMap(entt::entity lightEntity)
{
	if(const auto* comp = lightRegistry.try_get<PointShadowMapComponent>(lightEntity))
	{
		GLuint64 handle = glGetTextureHandleARB(comp->depthCubeMap);
		if(glIsTextureHandleResidentARB(handle))
			glMakeTextureHandleNonResidentARB(handle);

		if(comp->depthCubeMap)
			glDeleteTextures(1, &comp->depthCubeMap);
		if(comp->frameBuffer)
			glDeleteFramebuffers(1, &comp->frameBuffer);

		lightRegistry.remove<PointShadowMapComponent>(lightEntity);
	}
}

GLuint64 LightManager::createSpotShadowMap(entt::entity lightEntity, uint32_t width, uint32_t height)
{
	auto& comp = lightRegistry.emplace<SpotShadowMapComponent>(lightEntity);
	comp.shadowWidth = width;
	comp.shadowHeight = height;
	setupSpotShadowTexture(comp);

	const GLuint64 handle = glGetTextureHandleARB(comp.depthTexture);
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void LightManager::destroySpotShadowMap(entt::entity lightEntity)
{
	if(const auto* comp = lightRegistry.try_get<SpotShadowMapComponent>(lightEntity))
	{
		GLuint64 handle = glGetTextureHandleARB(comp->depthTexture);
		if(glIsTextureHandleResidentARB(handle))
			glMakeTextureHandleNonResidentARB(handle);

		if(comp->depthTexture)
			glDeleteTextures(1, &comp->depthTexture);
		if(comp->frameBuffer)
			glDeleteFramebuffers(1, &comp->frameBuffer);

		lightRegistry.remove<SpotShadowMapComponent>(lightEntity);
	}
}

GLuint64 LightManager::createDirShadowMap(entt::entity lightEntity, uint32_t width, uint32_t height)
{
	auto& comp = lightRegistry.emplace<DirShadowMapComponent>(lightEntity);
	comp.shadowWidth = width;
	comp.shadowHeight = height;
	setupDirShadowTexture(comp);

	const GLuint64 handle = glGetTextureHandleARB(comp.depthTexture);
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void LightManager::destroyDirShadowMap(entt::entity lightEntity)
{
	if(const auto* comp = lightRegistry.try_get<DirShadowMapComponent>(lightEntity))
	{
		GLuint64 handle = glGetTextureHandleARB(comp->depthTexture);
		if(glIsTextureHandleResidentARB(handle))
			glMakeTextureHandleNonResidentARB(handle);

		if(comp->depthTexture)
			glDeleteTextures(1, &comp->depthTexture);
		if(comp->frameBuffer)
			glDeleteFramebuffers(1, &comp->frameBuffer);

		lightRegistry.remove<DirShadowMapComponent>(lightEntity);
	}
}

std::array<mat4, 6> LightManager::getPointLightViewMatrices(const vec3& lightPos)
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

mat4 LightManager::getPointLightProjection(const float nearPlane, const float farPlane)
{
	return perspective(radians(90.0f), 1.0f, nearPlane, farPlane);
}

mat4 LightManager::getSpotLightSpaceMatrix(const vec3& position, const vec3& direction, float outerCutOff,
										   const float nearPlane, const float farPlane)
{
	float fov = acos(outerCutOff) * 2.0f;
	fov = glm::clamp(fov, glm::radians(10.0f), glm::radians(170.0f));

	const mat4 projection = perspective(fov, 1.0f, nearPlane, farPlane);

	// Handle case where direction is parallel to the default up vector
	const vec3 normalizedDir = normalize(direction);
	vec3 up = vec3(0.0f, 1.0f, 0.0f);
	if(abs(dot(normalizedDir, up)) > 0.99f)
		up = vec3(1.0f, 0.0f, 0.0f); // Use X-axis as up when light points straight up/down

	const mat4 view = lookAt(position, position + normalizedDir, up);

	return projection * view;
}

mat4 LightManager::getDirLightSpaceMatrix(const vec3& lightDir, const float orthoSize, const float nearPlane, const float farPlane)
{
	const mat4 lightProjection = ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
	const mat4 lightView = lookAt(-normalize(lightDir) * 25.0f, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
	return lightProjection * lightView;
}

void LightManager::setupPointShadowTexture(PointShadowMapComponent& comp)
{
	glGenFramebuffers(1, &comp.frameBuffer);
	glGenTextures(1, &comp.depthCubeMap);

	glBindTexture(GL_TEXTURE_CUBE_MAP, comp.depthCubeMap);
	for(unsigned int i = 0; i < 6; ++i)
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

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Point light shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightManager::setupSpotShadowTexture(SpotShadowMapComponent& comp)
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

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Spotlight shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightManager::setupDirShadowTexture(DirShadowMapComponent& comp)
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

	const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
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

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "ERROR: Directional light shadow map framebuffer is not complete!" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightManager::renderDirLightShadows(const DrawModelsCallback& drawModels)
{
	if(lightRegistry.storage<DirShadowMapComponent>().empty())
		return;

	glCullFace(GL_BACK);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	cachedShadowMapShader.use();

	auto view = lightRegistry.view<DirLightComponent, DirShadowMapComponent>();
	for(auto [entity, light, shadowComp] : view.each())
	{
		glViewport(0, 0, shadowComp.shadowWidth, shadowComp.shadowHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowComp.frameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);

		cachedShadowMapShader.setMat4("lightSpaceMatrix", light.lightSpaceMatrix);
		drawModels(cachedShadowMapShader);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightManager::renderPointLightShadows(const DrawModelsCallback& drawModels)
{
	if(lightRegistry.storage<PointShadowMapComponent>().empty())
		return;

	glCullFace(GL_BACK);

	cachedShadowPointShader.use();
	cachedShadowPointShader.setFloat("farPlane", POINT_LIGHT_FAR_PLANE);

	auto view = lightRegistry.view<PointLightComponent, PointShadowMapComponent>();
	for(auto [entity, light, shadowComp] : view.each())
	{
		glViewport(0, 0, shadowComp.shadowSize, shadowComp.shadowSize);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowComp.frameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);

		cachedShadowPointShader.setVec3("lightPos", light.position);

		for(int face = 0; face < 6; ++face)
		{
			string uniformName = "shadowMatrices[" + std::to_string(face) + "]";
			cachedShadowPointShader.setMat4(uniformName, light.shadowMatrices[face]);
		}

		drawModels(cachedShadowPointShader);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void LightManager::renderSpotlightShadows(const DrawModelsCallback& drawModels)
{
	if(lightRegistry.storage<SpotShadowMapComponent>().empty())
		return;

	glCullFace(GL_BACK);

	cachedShadowMapShader.use();

	auto view = lightRegistry.view<SpotlightComponent, SpotShadowMapComponent>();
	for(auto [entity, light, shadowComp] : view.each())
	{
		glViewport(0, 0, shadowComp.shadowWidth, shadowComp.shadowHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowComp.frameBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);

		cachedShadowMapShader.setMat4("lightSpaceMatrix", light.lightSpaceMatrix);
		drawModels(cachedShadowMapShader);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
