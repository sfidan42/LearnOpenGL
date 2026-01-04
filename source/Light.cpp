#include "Light.hpp"

#include <vector>
#include <glm/glm.hpp>

LightManager::LightManager(const Shader& mainShader, const Shader& skyShader)
: cachedMainShader(mainShader)
{
	// Create SSBOs for dynamic lights
	glGenBuffers(1, &pointLightSSBO);
	glGenBuffers(1, &spotLightSSBO);

	setupLightTracking();

	// Dawn: Sun is rising from the east, low angle
	// A Y-value close to 0.0 puts it on the horizon.
	sunLight.direction = vec3(-1.0f, -0.5f, 0.0f);
	// Warmer, dimmer ambient light (purplish-blue)
	sunLight.ambient = vec3(0.15f, 0.1f, 0.2f);
	// Strong orange/gold diffuse light
	sunLight.diffuse = vec3(1.0f, 0.5f, 0.2f);
	// Soft yellowish specular highlights
	sunLight.specular = vec3(0.8f, 0.7f, 0.3f);

	sendSunLight(mainShader, skyShader);
}

LightManager::~LightManager()
{
	if(pointLightSSBO)
		glDeleteBuffers(1, &pointLightSSBO);
	if(spotLightSSBO)
		glDeleteBuffers(1, &spotLightSSBO);
}

void LightManager::update(const float deltaTime, const Shader& mainShader, const Shader& skyShader)
{
	// Adjust speed as needed (e.g., 0.1f for a slow cycle)
	timeOfDay += deltaTime * 0.2f;

	// 1. Calculate Sun Position
	const float sunX = cosf(timeOfDay);
	const float sunZ = sinf(timeOfDay);
	const float sunY = sinf(timeOfDay);

	sunLight.direction = glm::normalize(vec3(sunX, sunY, sunZ));

	// 2. Define Time-of-Day Colors
	vec3 deepNight = vec3(0.01f, 0.01f, 0.02f);
	vec3 sunrise = vec3(1.0f, 0.4f, 0.1f);
	vec3 noon = vec3(1.0f, 1.0f, 0.9f);
	vec3 sunset = vec3(0.9f, 0.3f, 0.05f);

	// 3. Dynamic Color Interpolation based on sunY (height)
	if(sunY > 0.0f)
	{
		// Sun is above the horizon (Day/Sunrise/Sunset)
		// Interpolate between Sunset/Sunrise (low Y) and Noon (high Y)
		float factor = sunY; // 0.0 at horizon, 1.0 at peak
		sunLight.diffuse = glm::mix(sunrise, noon, factor);
		sunLight.ambient = sunLight.diffuse * 0.2f;
		sunLight.specular = sunLight.diffuse;
	}
	else
	{
		// Sun is below horizon (Night)
		// Fade from sunset color to deep night
		float factor = glm::clamp(sunY * -5.0f, 0.0f, 1.0f);
		sunLight.diffuse = glm::mix(sunset * 0.1f, deepNight, factor);
		sunLight.ambient = vec3(0.02f, 0.02f, 0.05f);
		sunLight.specular = vec3(0.0f);
	}

	sendSunLight(mainShader, skyShader);
}

PointLight LightManager::createPointLight(const vec3& position, const vec3& color)
{
	PointLightComponent lightComp{};
	lightComp.position = position;
	lightComp.constant = 1.0f;
	lightComp.ambient = color * 0.1f;
	lightComp.linear = 0.09f;
	lightComp.diffuse = color;
	lightComp.quadratic = 0.032f;
	lightComp.specular = color;
	lightComp._pad = 0.0f;
	const entt::entity lightEnt = lightRegistry.create();
	return {
		lightEnt,
		lightRegistry.emplace<PointLightComponent>(lightEnt, lightComp)
	};
}

SpotLight LightManager::createSpotLight(const vec3& position, const vec3& direction, const vec3& color)
{
	SpotLightComponent lightComp{};
	lightComp.position = position;
	lightComp.cutOff = glm::cos(glm::radians(12.5f));
	lightComp.direction = direction;
	lightComp.outerCutOff = glm::cos(glm::radians(17.5f));
	lightComp.ambient = color * 0.1f;
	lightComp.constant = 1.0f;
	lightComp.diffuse = color;
	lightComp.linear = 0.09f;
	lightComp.specular = color;
	lightComp.quadratic = 0.032f;
	const entt::entity lightEnt = lightRegistry.create();
	return {
		lightEnt,
		lightRegistry.emplace<SpotLightComponent>(lightEnt, lightComp)
	};
}

void LightManager::syncPointLight(const PointLight& light)
{
	lightRegistry.patch<PointLightComponent>(light.lightEntity);
}

void LightManager::syncSpotLight(const SpotLight& light)
{
	lightRegistry.patch<SpotLightComponent>(light.lightEntity);
}

void LightManager::setupLightTracking()
{
	// Point Lights
	lightRegistry.on_construct<PointLightComponent>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_update<PointLightComponent>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_destroy<PointLightComponent>().connect<&LightManager::onLightUpdate>(this);

	// Spot Lights
	lightRegistry.on_construct<SpotLightComponent>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_update<SpotLightComponent>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_destroy<SpotLightComponent>().connect<&LightManager::onLightUpdate>(this);
}

void LightManager::onLightUpdate(entt::registry& registry, entt::entity entity) const
{
	cachedMainShader.use();
	sendPointLights(cachedMainShader);
	sendSpotLights(cachedMainShader);
}

void LightManager::sendPointLights(const Shader& mainShader) const
{
	const uint32_t pLightsCount = lightRegistry.view<PointLightComponent>().size();
	mainShader.setInt("u_numPointLights", pLightsCount);

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
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pointLightSSBO);
}

void LightManager::sendSpotLights(const Shader& mainShader) const
{
	const uint32_t sLightsCount = lightRegistry.view<SpotLightComponent>().size();
	mainShader.setInt("u_numSpotLights", sLightsCount);

	vector<SpotLightComponent> spotLights;
	spotLights.reserve(sLightsCount);
	const auto& sLightView = lightRegistry.view<SpotLightComponent>();
	sLightView.each([&](const SpotLightComponent& light) { spotLights.push_back(light); });

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		sLightsCount * sizeof(SpotLightComponent),
		spotLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, spotLightSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightManager::sendSunLight(const Shader& mainShader, const Shader& skyShader) const
{
	mainShader.use();

	// Send directional light
	mainShader.setVec3("sunLight.direction", sunLight.direction);
	mainShader.setVec3("sunLight.ambient", sunLight.ambient);
	mainShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	mainShader.setVec3("sunLight.specular", sunLight.specular);

	skyShader.use();

	skyShader.setVec3("sunLight.direction", sunLight.direction);
	skyShader.setVec3("sunLight.ambient", sunLight.ambient);
	skyShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	skyShader.setVec3("sunLight.specular", sunLight.specular);
}
