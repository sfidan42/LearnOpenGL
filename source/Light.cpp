#include "Light.hpp"
#include <vector>
#include <glm/glm.hpp>

LightManager::LightManager()
{
	// Dawn: Sun is rising from the east, low angle
	// A Y-value close to 0.0 puts it on the horizon.
	dirLight.direction = vec3(-1.0f, 0.15f, 0.0f);
	// Warmer, dimmer ambient light (purplish-blue)
	dirLight.ambient = vec3(0.15f, 0.1f, 0.2f);
	// Strong orange/gold diffuse light
	dirLight.diffuse = vec3(1.0f, 0.5f, 0.2f);
	// Soft yellowish specular highlights
	dirLight.specular = vec3(0.8f, 0.7f, 0.3f);

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

void LightManager::send(const Shader& mainShader, const Shader& skyShader) const
{
	mainShader.use();

	// Send directional light
	mainShader.setVec3("dirLight.direction", dirLight.direction);
	mainShader.setVec3("dirLight.ambient", dirLight.ambient);
	mainShader.setVec3("dirLight.diffuse", dirLight.diffuse);
	mainShader.setVec3("dirLight.specular", dirLight.specular);

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

	// TODO: fix skybox lighting

	skyShader.use();

	skyShader.setVec3("dirLight.direction", dirLight.direction);
	skyShader.setVec3("dirLight.ambient", dirLight.ambient);
	skyShader.setVec3("dirLight.diffuse", dirLight.diffuse);
	skyShader.setVec3("dirLight.specular", dirLight.specular);
}
