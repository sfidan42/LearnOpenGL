#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Vertex.hpp"
#include "Shader.hpp"
#include "Model.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", nullptr, nullptr);
	if(window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	{
		std::vector<std::string> shader_filepaths;
		shader_filepaths.emplace_back("shaders/textured.shader");
		Shader shader;
		if(!shader.load_shaders(shader_filepaths))
		{
			std::cout << "Failed to load shaders" << std::endl;
			return -1;
		}

		std::vector<Model> models(2);

		for(auto& model : models)
		{
			if(!model.loadGeometry())
			{
				std::cout << "Failed to load geometry" << std::endl;
				return -1;
			}
		}

		if(!models[0].loadTexture("textures/container.jpg"))
			std::cout << "Failed to load texture for model 0" << std::endl;
		if(!models[1].loadTexture("textures/awesomeface.png"))
			std::cout << "Failed to load texture for model 1" << std::endl;

		for(auto& model : models)
			if(!model.bind())
				std::cout << "Failed to bind model" << std::endl;

		while(!glfwWindowShouldClose(window))
		{
			processInput(window);

			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			shader.use(0);

			for(auto& model : models)
				model.draw();

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	} // OpenGL objects destroyed here, before context termination

	glfwTerminate();
	return 0;
}

void processInput(GLFWwindow* window)
{
	if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
