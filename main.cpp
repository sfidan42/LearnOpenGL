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

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1200, 720, "LearnOpenGL", nullptr, nullptr);
	if(window == nullptr)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	stbi_set_flip_vertically_on_load(true);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1200, 720);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	{
		vector<string> shader_filepaths = {
			"shaders/programs.glsl",
		};

		Shader shader;
		if(!shader.loadShaders(shader_filepaths))
		{
			cout << "Failed to load shaders" << endl;
			return -1;
		}

		LightManager lightManager;

		string path(DATA_DIR);
		vector modelPaths = {
			"/models/backpack/backpack.obj",
			"/models/interior_tiles_1k.glb",
		};
		vector<Model> models;
		for(const auto& modelPath : modelPaths)
			models.emplace_back(path + modelPath);

		vector cubePositions = {
			vec3(0.0f, 0.0f, 0.0f),
			vec3(2.0f, 5.0f, -15.0f),
			vec3(-1.5f, -2.2f, -2.5f),
			vec3(-3.8f, -2.0f, -12.3f),
			vec3(2.4f, -0.4f, -3.5f),
			vec3(-1.7f, 3.0f, -7.5f),
			vec3(1.3f, -2.0f, -2.5f),
			vec3(1.5f, 2.0f, -2.5f),
			vec3(1.5f, 0.2f, -1.5f),
			vec3(-1.3f, 1.0f, -1.5f)
		};

		float lastTime = 0.0f;

		while(!glfwWindowShouldClose(window))
		{
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			float currentTime = static_cast<float>(glfwGetTime());
			float deltaTime = currentTime - lastTime;
			lastTime = currentTime;

			g_camera.update(deltaTime);

			shader.use(0);
			g_camera.send(shader);
			lightManager.send(shader);

			mat4 model;
			for(unsigned int i = 0; i < cubePositions.size(); i++)
			{
				model = mat4(1.0f);
				model = translate(model, cubePositions[i]);
				float angle = 20.0f * i;
				model = rotate(model, radians(angle), vec3(1.0f, 0.3f, 0.5f));
				model = scale(model, vec3(0.2f));
				shader.setMat4("model", model);
				models[0].draw(shader);
			}
			model = mat4(1.0f);
			model = translate(model, vec3(0.0f, -1.5f, 0.0f));
			model = scale(model, vec3(10.0f));
			shader.setMat4("model", model);
			models[1].draw(shader);

			glfwSwapBuffers(window);
			glfwPollEvents();
		}
	} // OpenGL objects destroyed here, before context termination

	glfwTerminate();
	glfwDestroyWindow(window);
	return 0;
}
