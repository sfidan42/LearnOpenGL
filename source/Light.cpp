#include "Light.hpp"
#include <vector>
#include <glm/glm.hpp>

LightManager::LightManager()
{
	// Dawn: Sun is rising from the east, low angle
	// A Y-value close to 0.0 puts it on the horizon.
	sunLight.direction = vec3(-1.0f, -0.5f, 0.0f);
	// Warmer, dimmer ambient light (purplish-blue)
	sunLight.ambient = vec3(0.15f, 0.1f, 0.2f);
	// Strong orange/gold diffuse light
	sunLight.diffuse = vec3(1.0f, 0.5f, 0.2f);
	// Soft yellowish specular highlights
	sunLight.specular = vec3(0.8f, 0.7f, 0.3f);

	pointLights.reserve(8);
	spotLights.reserve(8);

	// Point light in corner
	PointLightGPU pointLight{
		vec3(8.0f, 1.0f, 8.0f), // position
		1.0f, // constant
		vec3(0.3f, 0.3f, 0.3f), // ambient
		0.09f, // linear
		vec3(1.0f, 1.0f, 1.0f), // diffuse
		0.032f, // quadratic
		vec3(1.0f, 1.0f, 1.0f), // specular
		0.0f // padding
	};
	pointLights.push_back(pointLight);

	// Spotlights around center, pointing to floor center
	constexpr auto center = vec3(0.0f, -1.0f, 0.0f); // Floor center
	constexpr vec3 spotPositions[] = {
		vec3(-10.0f, 2.0f, -10.0f),
		vec3(-10.0f, 2.0f, 10.0f),
		vec3(10.0f, 2.0f, -10.0f)
	};
	constexpr vec3 spotColors[] = {
		vec3(1.0f, 0.0f, 0.0f), // Red
		vec3(0.0f, 1.0f, 0.0f), // Green
		vec3(0.0f, 0.0f, 1.0f) // Blue
	};
	for(int i = 0; i < 3; ++i)
	{
		SpotLightGPU spotLight{
			spotPositions[i],
			cos(radians(12.5f)), // cutOff
			normalize(center - spotPositions[i]),
			cos(radians(15.0f)), // outerCutOff
			spotColors[i] * 0.1f, // ambient
			1.0f, // constant
			spotColors[i], // diffuse
			0.09f, // linear
			spotColors[i], // specular
			0.032f // quadratic
		};
		spotLights.push_back(spotLight);
	}

	// Create SSBOs for dynamic lights
	glGenBuffers(1, &pointLightSSBO);
	glGenBuffers(1, &spotLightSSBO);
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
}

void LightManager::send(const Shader& mainShader, const Shader& skyShader) const
{
	mainShader.use();

	// Send directional light
	mainShader.setVec3("sunLight.direction", sunLight.direction);
	mainShader.setVec3("sunLight.ambient", sunLight.ambient);
	mainShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	mainShader.setVec3("sunLight.specular", sunLight.specular);

	// Update and bind SSBOs
	updateSSBOs();

	mainShader.setInt("u_numPointLights", pointLights.size());
	mainShader.setInt("u_numSpotLights", spotLights.size());

	skyShader.use();

	skyShader.setVec3("sunLight.direction", sunLight.direction);
	skyShader.setVec3("sunLight.ambient", sunLight.ambient);
	skyShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	skyShader.setVec3("sunLight.specular", sunLight.specular);
}

void LightManager::updateSSBOs() const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, pointLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		pointLights.size() * sizeof(PointLightGPU),
		pointLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pointLightSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, spotLightSSBO);
	glBufferData(
		GL_SHADER_STORAGE_BUFFER,
		spotLights.size() * sizeof(SpotLightGPU),
		spotLights.data(),
		GL_DYNAMIC_DRAW
	);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, spotLightSSBO);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}
