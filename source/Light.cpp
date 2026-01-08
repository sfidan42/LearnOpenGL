#include "Light.hpp"

#include <vector>
#include <glm/glm.hpp>

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

PointLightComponent& LightManager::getPointLight(const entt::entity entity)
{
	return lightRegistry.get<PointLightComponent>(entity);
}

SpotlightComponent& LightManager::getSpotlight(const entt::entity entity)
{
	return lightRegistry.get<SpotlightComponent>(entity);
}

DirLightComponent& LightManager::getDirLight(const entt::entity entity)
{
	assert(lightRegistry.all_of<DirLightComponent>(entity) && "Entity does not have DirLightComponent");
	assert(lightRegistry.valid(entity) && "Invalid entity passed to getDirLight");
	return lightRegistry.get<DirLightComponent>(entity);
}

void LightManager::updatePointLight(const entt::entity /*light*/)
{
	syncPointLights();
}

void LightManager::updateSpotlight(const entt::entity lightEntity)
{
	recalcSpotlightMatrix(lightEntity);
	syncSpotlights();
}

void LightManager::updateDirLight(const entt::entity lightEntity)
{
	recalcDirLightMatrix(lightEntity);
	syncDirLights();
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

void LightManager::recalcSpotlightMatrix(const entt::entity entity)
{
	const auto& [sLight, shadow] = lightRegistry.try_get<SpotlightComponent, SpotlightShadowMap>(entity);
	if(!sLight || !shadow)
		return;
	sLight->lightSpaceMatrix = shadow->getLightSpaceMatrix(
		sLight->position,
		normalize(sLight->direction),
		sLight->outerCutOff,
		0.1f, 50.0f
	);
}

void LightManager::recalcDirLightMatrix(const entt::entity entity)
{
	const auto& [dirLight, shadow] = lightRegistry.try_get<DirLightComponent, DirLightShadowMap>(entity);
	if(!dirLight || !shadow)
		return;
	dirLight->lightSpaceMatrix = DirLightShadowMap::getLightSpaceMatrix(
		dirLight->direction,
		50.0f, // ortho size - covers -50 to +50 on X/Y in light space
		0.1f, // near plane
		150.0f // far plane
	);
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

GLuint64 LightManager::createPointLightShadowMap(const entt::entity entity)
{
	const auto& shadowMap = lightRegistry.emplace<PointLightShadowMap>(entity, 512);
	const GLuint64 handle = glGetTextureHandleARB(shadowMap.getDepthCubemap());
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

GLuint64 LightManager::createSpotlightShadowMap(const entt::entity entity)
{
	const auto& shadowMap = lightRegistry.emplace<SpotlightShadowMap>(entity, 2048, 2048);
	const GLuint64 handle = glGetTextureHandleARB(shadowMap.getDepthTexture());
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

GLuint64 LightManager::createDirLightShadowMap(const entt::entity entity)
{
	const auto& shadowMap = lightRegistry.emplace<DirLightShadowMap>(entity, 2048, 2048);
	const GLuint64 handle = glGetTextureHandleARB(shadowMap.getDepthTexture());
	glMakeTextureHandleResidentARB(handle);
	return handle;
}

void LightManager::destroyPointLightShadowMap(const entt::entity entity)
{
	GLuint64& handle = lightRegistry.get<PointLightComponent>(entity).cubeMapHandle;
	handle = 0;
	glMakeTextureHandleResidentARB(handle);
	lightRegistry.remove<PointLightShadowMap>(entity);
}

void LightManager::destroySpotlightShadowMap(entt::entity entity)
{
	GLuint64& handle = lightRegistry.get<SpotlightComponent>(entity).shadowMapHandle;
	handle = 0;
	glMakeTextureHandleResidentARB(handle);
	lightRegistry.remove<SpotlightShadowMap>(entity);
}

void LightManager::destroyDirLightShadowMap(const entt::entity entity)
{
	GLuint64& handle = lightRegistry.get<DirLightComponent>(entity).shadowMapHandle;
	handle = 0;
	glMakeTextureHandleResidentARB(handle);
	lightRegistry.remove<DirLightShadowMap>(entity);
}
