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

	// Point light in corner
	pointLights.resize(1);
	pointLights[0].position = vec3(8.0f, 1.0f, 8.0f);
	pointLights[0].ambient = vec3(0.3f, 0.3f, 0.3f);
	pointLights[0].diffuse = vec3(1.0f, 1.0f, 1.0f);
	pointLights[0].specular = vec3(1.0f, 1.0f, 1.0f);
	pointLights[0].constant = 1.0f;
	pointLights[0].linear = 0.09f;
	pointLights[0].quadratic = 0.032f;

	// Spotlights around center, pointing to floor center
	spotLights.resize(3);
	constexpr auto center = vec3(0.0f, -1.0f, 0.0f); // Floor center
	constexpr vec3 spotPositions[] = {
		vec3(5.0f, 2.0f, 5.0f),
		vec3(-5.0f, 2.0f, 5.0f),
		vec3(0.0f, 2.0f, -5.0f)
	};
	constexpr vec3 spotColors[] = {
		vec3(1.0f, 0.0f, 0.0f), // Red
		vec3(0.0f, 1.0f, 0.0f), // Green
		vec3(0.0f, 0.0f, 1.0f) // Blue
	};
	for(int i = 0; i < 3; ++i)
	{
		spotLights[i].position = spotPositions[i];
		spotLights[i].direction = normalize(center - spotPositions[i]);
		spotLights[i].ambient = spotColors[i] * 0.1f;
		spotLights[i].diffuse = spotColors[i];
		spotLights[i].specular = spotColors[i];
		spotLights[i].constant = 1.0f;
		spotLights[i].linear = 0.09f;
		spotLights[i].quadratic = 0.032f;
		spotLights[i].cutOff = cos(radians(12.5f));
		spotLights[i].outerCutOff = cos(radians(15.0f));
	}
}

void LightManager::update(float deltaTime)
{
	// Adjust speed as needed (e.g., 0.1f for a slow cycle)
	m_time += deltaTime * 0.2f;

	// 1. Calculate Sun Position
	float sunX = cosf(m_time);
	float sunZ = sinf(m_time);
	float sunY = sinf(m_time);

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

	mainShader.setInt("u_numPointLights", static_cast<int>(pointLights.size()));
	// Send point lights
	for(size_t i = 0; i < pointLights.size(); ++i)
	{
		std::string base = "pointLights[" + std::to_string(i) + "]";
		mainShader.setVec3(base + ".position", pointLights[i].position);
		mainShader.setFloat(base + ".constant", pointLights[i].constant);
		mainShader.setFloat(base + ".linear", pointLights[i].linear);
		mainShader.setFloat(base + ".quadratic", pointLights[i].quadratic);
		mainShader.setVec3(base + ".ambient", pointLights[i].ambient);
		mainShader.setVec3(base + ".diffuse", pointLights[i].diffuse);
		mainShader.setVec3(base + ".specular", pointLights[i].specular);
	}

	mainShader.setInt("u_numSpotLights", static_cast<int>(spotLights.size()));
	// Send spotlights
	for(size_t i = 0; i < spotLights.size(); ++i)
	{
		std::string base = "spotLights[" + std::to_string(i) + "]";
		mainShader.setVec3(base + ".position", spotLights[i].position);
		mainShader.setVec3(base + ".direction", spotLights[i].direction);
		mainShader.setFloat(base + ".cutOff", spotLights[i].cutOff);
		mainShader.setFloat(base + ".outerCutOff", spotLights[i].outerCutOff);
		mainShader.setFloat(base + ".constant", spotLights[i].constant);
		mainShader.setFloat(base + ".linear", spotLights[i].linear);
		mainShader.setFloat(base + ".quadratic", spotLights[i].quadratic);
		mainShader.setVec3(base + ".ambient", spotLights[i].ambient);
		mainShader.setVec3(base + ".diffuse", spotLights[i].diffuse);
		mainShader.setVec3(base + ".specular", spotLights[i].specular);
	}

	skyShader.use();

	skyShader.setVec3("sunLight.direction", sunLight.direction);
	skyShader.setVec3("sunLight.ambient", sunLight.ambient);
	skyShader.setVec3("sunLight.diffuse", sunLight.diffuse);
	skyShader.setVec3("sunLight.specular", sunLight.specular);
}
