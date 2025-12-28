#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.hpp"
#include "Components.hpp"

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	bool init(const vector<string>& shaderFilepaths);
	void run();

	void loadModel(const string& modelPath, const TransformComponent& transform);

private:
	GLFWwindow* window = nullptr;
	Shader* shader;
	entt::registry modelRegistry;
};
