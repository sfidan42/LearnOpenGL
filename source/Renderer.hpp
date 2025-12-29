#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "Components.hpp"
#include "Light.hpp"

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
	entt::registry modelRegistry;
	LightManager lightManager;
};
