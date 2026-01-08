#include "Light.hpp"
#include <vector>
#include <glm/glm.hpp>
#include "Shadow.hpp"

LightManager::LightManager(const Shader& mainShader, const Shader& skyShader)
: cachedMainShader(mainShader), cachedSkyShader(skyShader)
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
	lightComp.farPlane = PointLightShadowMap::FAR_PLANE;
	lightComp.cubeMapHandle = 0; // Will be set after shadow map creation

	const entt::entity lightEnt = lightRegistry.create();
	auto& comp = lightRegistry.emplace<PointLightComponent>(lightEnt, lightComp);

	// Create shadow map for this light
	comp.cubeMapHandle = createPointLightShadowMap(lightEnt);

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

	// Create shadow map for this light
	comp.shadowMapHandle = createSpotlightShadowMap(lightEnt);

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

	comp.shadowMapHandle = createDirLightShadowMap(lightEnt);

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
	destroyPointLightShadowMap(lightEntity);
	lightRegistry.destroy(lightEntity);
	syncPointLights();
}

void LightManager::deleteSpotlight(const entt::entity lightEntity)
{
	destroySpotlightShadowMap(lightEntity);
	lightRegistry.destroy(lightEntity);
	syncSpotlights();
}

void LightManager::deleteDirLight(const entt::entity lightEntity)
{
	destroyDirLightShadowMap(lightEntity);
	lightRegistry.destroy(lightEntity);
	syncDirLights();
}

void LightManager::recalcSpotlightMatrix(const entt::entity lightEntity)
{
	const auto& [sLight, shadow]
		= lightRegistry.try_get<SpotlightComponent, SpotlightShadowMap>(lightEntity);
	if(!sLight || !shadow)
		return;

	sLight->lightSpaceMatrix = shadow->getLightSpaceMatrix(
		sLight->position, normalize(sLight->direction),
		sLight->outerCutOff, 0.1f, 50.0f
	);
}

void LightManager::recalcDirLightMatrix(const entt::entity lightEntity)
{
	const auto& [dirLight, shadow] = lightRegistry.try_get<DirLightComponent, DirLightShadowMap>(lightEntity);
	if(!dirLight || !shadow)
		return;
	dirLight->lightSpaceMatrix = shadow->getLightSpaceMatrix(
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

GLuint64 LightManager::createPointLightShadowMap(const entt::entity lightEntity)
{
	const auto& shadowMap = lightRegistry.emplace<PointLightShadowMap>(lightEntity, 512);
	const GLuint64 handle = glGetTextureHandleARB(shadowMap.getDepthCubemap());
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

GLuint64 LightManager::createSpotlightShadowMap(const entt::entity lightEntity)
{
	const auto& shadowMap = lightRegistry.emplace<SpotlightShadowMap>(lightEntity, 2048, 2048);
	const GLuint64 handle = glGetTextureHandleARB(shadowMap.getDepthTexture());
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

GLuint64 LightManager::createDirLightShadowMap(const entt::entity lightEntity)
{
	const auto& shadowMap = lightRegistry.emplace<DirLightShadowMap>(lightEntity, 2048, 2048);
	const GLuint64 handle = glGetTextureHandleARB(shadowMap.getDepthTexture());
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void LightManager::destroyPointLightShadowMap(const entt::entity lightEntity)
{
	GLuint64& handle = lightRegistry.get<PointLightComponent>(lightEntity).cubeMapHandle;
	handle = 0;
	glMakeTextureHandleResidentARB(handle);
	lightRegistry.remove<PointLightShadowMap>(lightEntity);
}

void LightManager::destroySpotlightShadowMap(entt::entity lightEntity)
{
	GLuint64& handle = lightRegistry.get<SpotlightComponent>(lightEntity).shadowMapHandle;
	handle = 0;
	glMakeTextureHandleResidentARB(handle);
	lightRegistry.remove<SpotlightShadowMap>(lightEntity);
}

void LightManager::destroyDirLightShadowMap(const entt::entity lightEntity)
{
	GLuint64& handle = lightRegistry.get<DirLightComponent>(lightEntity).shadowMapHandle;
	handle = 0;
	glMakeTextureHandleResidentARB(handle);
	lightRegistry.remove<DirLightShadowMap>(lightEntity);
}
