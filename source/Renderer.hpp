#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "Components.hpp"
#include "Light.hpp"
#include "Skybox.hpp"

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	bool init(const string& mainShaderPath, const string& skyboxShaderPath);
	void run();

	void loadModel(const string& modelPath, const TransformComponent& transform);

private:
	GLFWwindow* window = nullptr;
	Shader* mainShader = nullptr;
	Shader* skyboxShader = nullptr;
	Skybox* skybox = nullptr;
	entt::registry modelRegistry;
	LightManager* lightManager = nullptr;
};
