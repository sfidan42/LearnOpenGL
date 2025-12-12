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

	// Spot lights around center, pointing to floor center
	spotLights.resize(3);
	glm::vec3 center = glm::vec3(0.0f, -1.0f, 0.0f); // Floor center
	glm::vec3 spotPositions[] = {
		glm::vec3(5.0f, 2.0f, 5.0f),
		glm::vec3(-5.0f, 2.0f, 5.0f),
		glm::vec3(0.0f, 2.0f, -5.0f)
	};
	glm::vec3 spotColors[] = {
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
	shader.set3f("dirLight.direction", dirLight.direction.x, dirLight.direction.y, dirLight.direction.z);
	shader.set3f("dirLight.ambient", dirLight.ambient.x, dirLight.ambient.y, dirLight.ambient.z);
	shader.set3f("dirLight.diffuse", dirLight.diffuse.x, dirLight.diffuse.y, dirLight.diffuse.z);
	shader.set3f("dirLight.specular", dirLight.specular.x, dirLight.specular.y, dirLight.specular.z);

	// Send point lights
	for(size_t i = 0; i < pointLights.size(); ++i)
	{
		std::string base = "pointLights[" + std::to_string(i) + "]";
		shader.set3f((base + ".position").c_str(), pointLights[i].position.x, pointLights[i].position.y,
					 pointLights[i].position.z);
		shader.set1f((base + ".constant").c_str(), pointLights[i].constant);
		shader.set1f((base + ".linear").c_str(), pointLights[i].linear);
		shader.set1f((base + ".quadratic").c_str(), pointLights[i].quadratic);
		shader.set3f((base + ".ambient").c_str(), pointLights[i].ambient.x, pointLights[i].ambient.y,
					 pointLights[i].ambient.z);
		shader.set3f((base + ".diffuse").c_str(), pointLights[i].diffuse.x, pointLights[i].diffuse.y,
					 pointLights[i].diffuse.z);
		shader.set3f((base + ".specular").c_str(), pointLights[i].specular.x, pointLights[i].specular.y,
					 pointLights[i].specular.z);
	}

	// Send spot lights
	for(size_t i = 0; i < spotLights.size(); ++i)
	{
		std::string base = "spotLights[" + std::to_string(i) + "]";
		shader.set3f((base + ".position").c_str(), spotLights[i].position.x, spotLights[i].position.y,
					 spotLights[i].position.z);
		shader.set3f((base + ".direction").c_str(), spotLights[i].direction.x, spotLights[i].direction.y,
					 spotLights[i].direction.z);
		shader.set1f((base + ".cutOff").c_str(), spotLights[i].cutOff);
		shader.set1f((base + ".outerCutOff").c_str(), spotLights[i].outerCutOff);
		shader.set1f((base + ".constant").c_str(), spotLights[i].constant);
		shader.set1f((base + ".linear").c_str(), spotLights[i].linear);
		shader.set1f((base + ".quadratic").c_str(), spotLights[i].quadratic);
		shader.set3f((base + ".ambient").c_str(), spotLights[i].ambient.x, spotLights[i].ambient.y,
					 spotLights[i].ambient.z);
		shader.set3f((base + ".diffuse").c_str(), spotLights[i].diffuse.x, spotLights[i].diffuse.y,
					 spotLights[i].diffuse.z);
		shader.set3f((base + ".specular").c_str(), spotLights[i].specular.x, spotLights[i].specular.y,
					 spotLights[i].specular.z);
	}
}
