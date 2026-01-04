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

PointLightGPU& LightManager::createPointLight(const vec3& position, const vec3& color)
{
	entt::entity entity = lightRegistry.create();
	PointLightGPU light{};
	light.position = position;
	light.constant = 1.0f;
	light.ambient = color * 0.1f;
	light.linear = 0.09f;
	light.diffuse = color;
	light.quadratic = 0.032f;
	light.specular = color;
	light._pad = 0.0f;
	// Emplace fully initialized light - callback will have valid data
	return lightRegistry.emplace<PointLightGPU>(entity, light);
}

SpotLightGPU& LightManager::createSpotLight(const vec3& position, const vec3& direction, const vec3& color)
{
	SpotLightGPU sLight{
		position,
		cos(radians(12.5f)),
		normalize(direction),
		cos(radians(15.0f)),
		color * 0.1f,
		1.0f,
		color,
		0.09f,
		color,
		0.032f
	};
	return lightRegistry.emplace<SpotLightGPU>(lightRegistry.create(), sLight);
}

void LightManager::setupLightTracking()
{
	// Point Lights
	lightRegistry.on_construct<PointLightGPU>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_update<PointLightGPU>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_destroy<PointLightGPU>().connect<&LightManager::onLightUpdate>(this);

	// Spot Lights
	lightRegistry.on_construct<SpotLightGPU>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_update<SpotLightGPU>().connect<&LightManager::onLightUpdate>(this);
	lightRegistry.on_destroy<SpotLightGPU>().connect<&LightManager::onLightUpdate>(this);
}

void LightManager::onLightUpdate(entt::registry& registry, entt::entity entity) const
{
	cachedMainShader.use();
	sendPointLights(cachedMainShader);
	sendSpotLights(cachedMainShader);
}

void LightManager::sendPointLights(const Shader& mainShader) const
{
	const uint32_t pLightsCount = lightRegistry.view<PointLightGPU>().size();
	mainShader.setInt("u_numPointLights", pLightsCount);

	vector<PointLightGPU> pointLights;
	pointLights.reserve(pLightsCount);
	const auto& pLightView = lightRegistry.view<PointLightGPU>();
	pLightView.each([&](const PointLightGPU& light) { pointLights.push_back(light); });

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		pLightsCount * sizeof(PointLightGPU),
		pointLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pointLightSSBO);
}

void LightManager::sendSpotLights(const Shader& mainShader) const
{
	const uint32_t sLightsCount = lightRegistry.view<SpotLightGPU>().size();
	mainShader.setInt("u_numSpotLights", sLightsCount);

	vector<SpotLightGPU> spotLights;
	spotLights.reserve(sLightsCount);
	const auto& sLightView = lightRegistry.view<SpotLightGPU>();
	sLightView.each([&](const SpotLightGPU& light) { spotLights.push_back(light); });

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		sLightsCount * sizeof(SpotLightGPU),
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
