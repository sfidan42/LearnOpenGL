#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.hpp"
#include "Model.hpp"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Light.hpp"
#include "callbacks.hpp"
#include <stb_image.h>

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	bool init(const vector<string>& shaderFilepaths);
	void run();

private:

	GLFWwindow* window = nullptr;
	Shader* shader;
};
