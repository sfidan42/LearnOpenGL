#include "Light.hpp"
#include <vector>
#include <glm/glm.hpp>

LightManager::LightManager()
{
	// Directional light from bottom
	dirLight.direction = glm::vec3(0.0f, 1.0f, 0.0f);
	dirLight.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
	dirLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
	dirLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);

	// Point light in corner
	pointLights.resize(1);
	pointLights[0].position = glm::vec3(5.0f, 1.0f, 5.0f);
	pointLights[0].ambient = glm::vec3(0.3f, 0.3f, 0.3f);
	pointLights[0].diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
	pointLights[0].specular = glm::vec3(1.0f, 1.0f, 1.0f);
	pointLights[0].constant = 1.0f;
	pointLights[0].linear = 0.09f;
	pointLights[0].quadratic = 0.032f;

	// Spotlights around center, pointing to floor center
	spotLights.resize(3);
	constexpr glm::vec3 center = glm::vec3(0.0f, -1.0f, 0.0f); // Floor center
	constexpr glm::vec3 spotPositions[] = {
		glm::vec3(5.0f, 2.0f, 5.0f),
		glm::vec3(-5.0f, 2.0f, 5.0f),
		glm::vec3(0.0f, 2.0f, -5.0f)
	};
	constexpr glm::vec3 spotColors[] = {
		glm::vec3(1.0f, 0.0f, 0.0f), // Red
		glm::vec3(0.0f, 1.0f, 0.0f), // Green
		glm::vec3(0.0f, 0.0f, 1.0f) // Blue
	};
	for(int i = 0; i < 3; ++i)
	{
		spotLights[i].position = spotPositions[i];
		spotLights[i].direction = glm::normalize(center - spotPositions[i]);
		spotLights[i].ambient = spotColors[i] * 0.1f;
		spotLights[i].diffuse = spotColors[i];
		spotLights[i].specular = spotColors[i];
		spotLights[i].constant = 1.0f;
		spotLights[i].linear = 0.09f;
		spotLights[i].quadratic = 0.032f;
		spotLights[i].cutOff = glm::cos(glm::radians(12.5f));
		spotLights[i].outerCutOff = glm::cos(glm::radians(15.0f));
	}
}

void LightManager::send(const Shader& shader) const
{
	// Send directional light
	shader.setVec3("dirLight.direction", dirLight.direction);
	shader.setVec3("dirLight.ambient", dirLight.ambient);
	shader.setVec3("dirLight.diffuse", dirLight.diffuse);
	shader.setVec3("dirLight.specular", dirLight.specular);

	// Send point lights
	for(size_t i = 0; i < pointLights.size(); ++i)
	{
		std::string base = "pointLights[" + std::to_string(i) + "]";
		shader.setVec3(base + ".position", pointLights[i].position);
		shader.setFloat(base + ".constant", pointLights[i].constant);
		shader.setFloat(base + ".linear", pointLights[i].linear);
		shader.setFloat(base + ".quadratic", pointLights[i].quadratic);
		shader.setVec3(base + ".ambient", pointLights[i].ambient);
		shader.setVec3(base + ".diffuse", pointLights[i].diffuse);
		shader.setVec3(base + ".specular", pointLights[i].specular);
	}

	// Send spotlights
	for(size_t i = 0; i < spotLights.size(); ++i)
	{
		std::string base = "spotLights[" + std::to_string(i) + "]";
		shader.setVec3(base + ".position", spotLights[i].position);
		shader.setVec3(base + ".direction", spotLights[i].direction);
		shader.setFloat(base + ".cutOff", spotLights[i].cutOff);
		shader.setFloat(base + ".outerCutOff", spotLights[i].outerCutOff);
		shader.setFloat(base + ".constant", spotLights[i].constant);
		shader.setFloat(base + ".linear", spotLights[i].linear);
		shader.setFloat(base + ".quadratic", spotLights[i].quadratic);
		shader.setVec3(base + ".ambient", spotLights[i].ambient);
		shader.setVec3(base + ".diffuse", spotLights[i].diffuse);
		shader.setVec3(base + ".specular", spotLights[i].specular);
	}
}
