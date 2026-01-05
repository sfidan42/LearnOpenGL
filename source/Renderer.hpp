#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

	bool init(const string& mainShaderPath, const string& skyboxShaderPath, const string& shadowShaderPath);
	void run();

	void loadModel(const string& modelPath, const TransformComponent& transform);

	LightManager* lightManager = nullptr;

private:
	void renderShadowPass();
	void renderScene();

	GLFWwindow* window = nullptr;
	Shader* mainShader = nullptr;
	Shader* skyboxShader = nullptr;
	Shader* shadowShader = nullptr;
	Skybox* skybox = nullptr;
	ShadowMap* shadowMap = nullptr;
	entt::registry modelRegistry;

	int windowWidth = 1200;
	int windowHeight = 720;
};
