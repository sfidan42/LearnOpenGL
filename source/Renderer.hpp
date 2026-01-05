#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Shader.hpp"
#include "Components.hpp"
#include "Light.hpp"
#include "Skybox.hpp"
#include "ShadowMap.hpp"

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	bool init(const string& mainShaderPath, const string& skyboxShaderPath);
	void run();

	void loadModel(const string& modelPath, const TransformComponent& transform);

	LightManager* lightManager = nullptr;

private:
	void renderShadowPass();
	void renderPointLightShadows();
	void renderSpotLightShadows();
	void renderScene();

	GLFWwindow* window = nullptr;
	Shader* mainShader = nullptr;
	Shader* skyboxShader = nullptr;
	Shader* shadowMapShader = nullptr;
	Shader* shadowPointShader = nullptr;
	Skybox* skybox = nullptr;
	ShadowMap* shadowMap = nullptr;

	// Shadow maps for dynamic lights
	std::vector<PointLightShadowMap> pointLightShadowMaps;
	std::vector<SpotLightShadowMap> spotLightShadowMaps;

	entt::registry modelRegistry;

	int windowWidth = 1200;
	int windowHeight = 720;
};
