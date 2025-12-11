#include "Light.hpp"
#include <vector>

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
