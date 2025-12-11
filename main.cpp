#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.hpp"
#include "Model.hpp"
#include "Camera.hpp"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Light.hpp"
#include "Material.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

Camera camera;

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1200, 720, "LearnOpenGL", nullptr, nullptr);
	if(window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1200, 720);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	{
		std::vector<std::string> shader_filepaths = {
			"shaders/multiple_lights.shader",
		};

		Shader shader;
		if(!shader.load_shaders(shader_filepaths))
		{
			std::cout << "Failed to load shaders" << std::endl;
			return -1;
		}

		LightManager lightManager;

		// Directional light from bottom
		lightManager.dirLight.direction = glm::vec3(0.0f, 1.0f, 0.0f);
		lightManager.dirLight.ambient = glm::vec3(0.1f, 0.1f, 0.1f);
		lightManager.dirLight.diffuse = glm::vec3(0.5f, 0.5f, 0.5f);
		lightManager.dirLight.specular = glm::vec3(0.5f, 0.5f, 0.5f);

		// Point light in corner
		lightManager.pointLights.resize(1);
		lightManager.pointLights[0].position = glm::vec3(10.0f, 2.0f, 10.0f);
		lightManager.pointLights[0].ambient = glm::vec3(0.1f, 0.1f, 0.1f);
		lightManager.pointLights[0].diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
		lightManager.pointLights[0].specular = glm::vec3(1.0f, 1.0f, 1.0f);
		lightManager.pointLights[0].constant = 1.0f;
		lightManager.pointLights[0].linear = 0.09f;
		lightManager.pointLights[0].quadratic = 0.032f;

		// Spot lights around center, pointing to floor center
		lightManager.spotLights.resize(3);
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
			lightManager.spotLights[i].position = spotPositions[i];
			lightManager.spotLights[i].direction = glm::normalize(center - spotPositions[i]);
			lightManager.spotLights[i].ambient = spotColors[i] * 0.1f;
			lightManager.spotLights[i].diffuse = spotColors[i];
			lightManager.spotLights[i].specular = spotColors[i];
			lightManager.spotLights[i].constant = 1.0f;
			lightManager.spotLights[i].linear = 0.09f;
			lightManager.spotLights[i].quadratic = 0.032f;
			lightManager.spotLights[i].cutOff = glm::cos(glm::radians(12.5f));
			lightManager.spotLights[i].outerCutOff = glm::cos(glm::radians(15.0f));
		}

		std::vector<Model> models(3);

		for(auto& model : models)
		{
			if(!model.loadGeometry())
			{
				std::cout << "Failed to load geometry" << std::endl;
				return -1;
			}
		}

		std::vector<std::string> textureFiles = {
			"textures/awesomeface.png",
			"textures/container2.png",
			"textures/container2_specular.png",
			"textures/white.png"
		};

		std::vector<Texture> textures(textureFiles.size());

		for(int i = 0; i < textureFiles.size(); i++)
		{
			if(!textures[i].load(textureFiles[i]))
			{
				std::cout << "Failed to load texture: " << textureFiles[i] << std::endl;
				return -1;
			}
		}

		std::shared_ptr<Material> materials[3];
		materials[0] = std::make_shared<Material>(textures[0], textures[0]);
		materials[1] = std::make_shared<Material>(textures[1], textures[2]);
		materials[2] = std::make_shared<Material>(textures[3], textures[3]); // Floor material

		materials[0]->shininess = 32.0f;
		materials[0]->ambient = glm::vec3(0.5f, 0.5f, 0.5f);

		materials[1]->shininess = 128.0f;
		materials[1]->ambient = glm::vec3(0.2f, 0.2f, 0.2f);

		materials[2]->shininess = 32.0f;
		materials[2]->ambient = glm::vec3(0.5f, 0.5f, 0.5f);

		models[0].setMaterial(materials[0]);
		models[1].setMaterial(materials[1]);
		models[2].setMaterial(materials[2]); // Floor model

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
		inst.scale = glm::vec3(0.80f);

		for(int i = 0; i < cubePositions.size(); i++)
		{
			int m = i % 2;
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

		// Instantiate floor
		inst.translation = glm::vec3(0.0f, -1.0f, 0.0f);
		inst.rotation = glm::vec3(0.0f);
		inst.scale = glm::vec3(20.0f, 0.01f, 20.0f);
		if(!models[2].instantiate(inst))
		{
			std::cout << "Failed to instantiate floor model" << std::endl;
			return -1;
		}

		shader.use(0);
		lightManager.send(shader);
		for(auto& model : models)
			model.send(shader);

		float lastTime = 0.0f;

		while(!glfwWindowShouldClose(window))
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			float currentTime = static_cast<float>(glfwGetTime());
			float deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			camera.update(deltaTime);

			shader.use(0);
			camera.send(shader);
			lightManager.send(shader);

			for(auto& model : models)
			{
				model.bind();
				model.update(deltaTime);
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
	camera.setAspect(width, height);
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
					camera.speed.z += 1.0f;
					break;
				case GLFW_KEY_S:
					camera.speed.z -= 1.0f;
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
					camera.speed.z -= 1.0f;
					break;
				case GLFW_KEY_S:
					camera.speed.z += 1.0f;
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

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	static float lastX = 400.0f;
	static float lastY = 300.0f;
	static bool firstMouse = true;

	if(firstMouse)
	{
		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);
		firstMouse = false;
	}

	float xoffset = static_cast<float>(xpos) - lastX;
	float yoffset = lastY - static_cast<float>(ypos); // reversed since y-coordinates go from bottom to top

	lastX = static_cast<float>(xpos);
	lastY = static_cast<float>(ypos);

	camera.mouse(xoffset, yoffset);
}
