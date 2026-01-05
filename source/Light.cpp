#include "Light.hpp"

#include <vector>
#include <glm/glm.hpp>

PointLight::PointLight(PointLightComponent& component, const entt::entity entity)
: lightData(component), lightEntity(entity)
{}

SpotLight::SpotLight(SpotLightComponent& component, const entt::entity entity)
: lightData(component), lightEntity(entity)
{}

LightManager::LightManager(const Shader& mainShader, const Shader& skyShader)
: cachedMainShader(mainShader), cachedSkyShader(skyShader)
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

	syncSunLight();
}

LightManager::~LightManager()
{
	if(pointLightSSBO)
		glDeleteBuffers(1, &pointLightSSBO);
	if(spotLightSSBO)
		glDeleteBuffers(1, &spotLightSSBO);
}

void LightManager::update(const float deltaTime)
{
	// Adjust speed as needed (e.g., 0.1f for a slow cycle)
	timeOfDay += deltaTime * 0.2f;

	// 1. Calculate Sun Position
	// The sun orbits around, with sunY indicating height above/below horizon
	const float sunX = cosf(timeOfDay);
	const float sunZ = sinf(timeOfDay);
	const float sunY = sinf(timeOfDay);

	// sunLight.direction should point FROM the sun TO the scene (downward when sun is up)
	// When sunY > 0, the sun is above the horizon, so light should come from above (negative Y direction)
	sunLight.direction = normalize(-vec3(sunX, sunY, sunZ));

	// 2. Define Time-of-Day Colors
	constexpr auto deepNight = vec3(0.01f, 0.01f, 0.02f);
	constexpr auto sunrise = vec3(1.0f, 0.4f, 0.1f);
	constexpr auto noon = vec3(1.0f, 1.0f, 0.9f);
	constexpr auto sunset = vec3(0.9f, 0.3f, 0.05f);

	// 3. Dynamic Color Interpolation based on sunY (height)
	if(sunY > 0.0f)
	{
		// Sun is above the horizon (Day/Sunrise/Sunset)
		// Interpolate between Sunset/Sunrise (low Y) and Noon (high Y)
		const float factor = sunY; // 0.0 at horizon, 1.0 at peak
		sunLight.diffuse = mix(sunrise, noon, factor);
		sunLight.ambient = sunLight.diffuse * 0.2f;
		sunLight.specular = sunLight.diffuse;
	}
	else
	{
		// Sun is below horizon (Night)
		// Fade from sunset color to deep night
		const float factor = glm::clamp(sunY * -5.0f, 0.0f, 1.0f);
		sunLight.diffuse = mix(sunset * 0.1f, deepNight, factor);
		sunLight.ambient = vec3(0.02f, 0.02f, 0.05f);
		sunLight.specular = vec3(0.0f);
	}

	syncSunLight();
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
		lightRegistry.emplace<PointLightComponent>(lightEnt, lightComp),
		lightEnt
	};
}

SpotLight LightManager::createSpotLight(const vec3& position, const vec3& direction, const vec3& color)
{
	SpotLightComponent lightComp{};
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
	const entt::entity lightEnt = lightRegistry.create();
	return {
		lightRegistry.emplace<SpotLightComponent>(lightEnt, lightComp),
		lightEnt
	};
}

void LightManager::updatePointLight(const PointLight& light)
{
	lightRegistry.patch<PointLightComponent>(light.lightEntity);
}

void LightManager::updateSpotLight(const SpotLight& light)
{
	lightRegistry.patch<SpotLightComponent>(light.lightEntity);
}

void LightManager::deletePointLight(const PointLight& light)
{
	lightRegistry.destroy(light.lightEntity);
	syncPointLights(lightRegistry, light.lightEntity);
}

void LightManager::deleteSpotLight(const SpotLight& light)
{
	lightRegistry.destroy(light.lightEntity);
	syncSpotLights(lightRegistry, light.lightEntity);
}

void LightManager::setupLightTracking()
{
	// Connect the reactive storage to the registry and set up callbacks
	lightRegistry.on_construct<PointLightComponent>().connect<&LightManager::syncPointLights>(*this);
	lightRegistry.on_update<PointLightComponent>().connect<&LightManager::syncPointLights>(*this);

	lightRegistry.on_construct<SpotLightComponent>().connect<&LightManager::syncSpotLights>(*this);
	lightRegistry.on_update<SpotLightComponent>().connect<&LightManager::syncSpotLights>(*this);
}

void LightManager::syncPointLights(entt::registry& /*registry*/, entt::entity /*entity*/)
{
	cachedMainShader.use();
	const uint32_t pLightsCount = lightRegistry.view<PointLightComponent>().size();
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
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pointLightSSBO);
}

void LightManager::syncSpotLights(entt::registry& /*registry*/, entt::entity /*entity*/)
{
	cachedMainShader.use();
	const uint32_t sLightsCount = lightRegistry.view<SpotLightComponent>().size();
	cachedMainShader.setInt("u_numSpotLights", sLightsCount);

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

void LightManager::syncSunLight() const
{
	cachedMainShader.use();

	// Send directional light
	cachedMainShader.setVec3("sunLight.direction", sunLight.direction);
	cachedMainShader.setVec3("sunLight.ambient", sunLight.ambient);
	cachedMainShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	cachedMainShader.setVec3("sunLight.specular", sunLight.specular);

	cachedSkyShader.use();

	cachedSkyShader.setVec3("sunLight.direction", sunLight.direction);
	cachedSkyShader.setVec3("sunLight.ambient", sunLight.ambient);
	cachedSkyShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	cachedSkyShader.setVec3("sunLight.specular", sunLight.specular);
}
