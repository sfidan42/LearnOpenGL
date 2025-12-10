#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Vertex.hpp"
#include "Shader.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

Camera camera;

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

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 800, 600);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

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
		{
			std::cout << "Failed to load texture for model 0" << std::endl;
			return -1;
		}

		if(!models[1].loadTexture("textures/awesomeface.png"))
		{
			std::cout << "Failed to load texture for model 1" << std::endl;
			return -1;
		}

		for(auto& model : models)
		{
			if(!model.bind())
			{
				std::cout << "Failed to bind model" << std::endl;
				return -1;
			}
		}

		std::vector cubePositions = {
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(2.0f, 5.0f, -15.0f),
			glm::vec3(-1.5f, -2.2f, -2.5f),
			glm::vec3(-3.8f, -2.0f, -12.3f),
			glm::vec3(2.4f, -0.4f, -3.5f),
			glm::vec3(-1.7f, 3.0f, -7.5f),
			glm::vec3(1.3f, -2.0f, -2.5f),
			glm::vec3(1.5f, 2.0f, -2.5f),
			glm::vec3(1.5f, 0.2f, -1.5f),
			glm::vec3(-1.3f, 1.0f, -1.5f)
		};

		InstanceData inst{};
		inst.translation = glm::vec3(-0.5f, -0.5f, 0.0f);
		inst.rotation = glm::vec3(0.0f);
		inst.scale = glm::vec3(0.4f);


		for(int i = 0; i < cubePositions.size(); i++)
		{
			int m = i % 2;

			for(int j = 0; j < 2; j++)
			{
				inst.translation = cubePositions[i];
				float angle = 20.0f * i;
				glm::vec3 axis = glm::vec3(1.0f, 0.3f, 0.5f);
				glm::quat quat = glm::angleAxis(glm::radians(angle), glm::normalize(axis));
				inst.rotation = glm::eulerAngles(quat);
				if(!models[m].instantiate(inst))
				{
					std::cout << "Failed to instantiate model" << std::endl;
					return -1;
				}
			}
			inst.translation += glm::vec3(0.0f, 0.5f, 0.0f);
		}

		while(!glfwWindowShouldClose(window))
		{
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			shader.use(0);
			static double deltaTime = glfwGetTime();
			deltaTime = glfwGetTime() - deltaTime;

			camera.update(static_cast<float>(deltaTime));

			glm::mat4 proj = camera.getProj();
			shader.setMat4fv("projection", &proj[0][0]);
			glm::mat4 view = camera.getView();
			shader.setMat4fv("view", &view[0][0]);

			for(auto& model : models)
			{
				model.update();
				model.draw();
			}

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	} // OpenGL objects destroyed here, before context termination

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch(action)
	{
		case GLFW_PRESS:
			switch(key)
			{
				case GLFW_KEY_ESCAPE:
					glfwSetWindowShouldClose(window, true);
					break;
				case GLFW_KEY_W:
					camera.speed.z -= 1.0f;
					break;
				case GLFW_KEY_S:
					camera.speed.z += 1.0f;
					break;
				case GLFW_KEY_A:
					camera.speed.x -= 1.0f;
					break;
				case GLFW_KEY_D:
					camera.speed.x += 1.0f;
					break;
				default:
					break;
			}
			break;
		case GLFW_RELEASE:
			switch(key)
			{
				case GLFW_KEY_W:
					camera.speed.z += 1.0f;
					break;
				case GLFW_KEY_S:
					camera.speed.z -= 1.0f;
					break;
				case GLFW_KEY_A:
					camera.speed.x += 1.0f;
					break;
				case GLFW_KEY_D:
					camera.speed.x -= 1.0f;
					break;
				default:
					break;
			}
			break;
		default:
			std::cout << "undefined action" << std::endl;
			break;
	}
}
