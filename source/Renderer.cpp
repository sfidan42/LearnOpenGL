#include "Renderer.hpp"

#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Light.hpp"
#include "callbacks.hpp"
#include <stb_image.h>
#include <iostream>

Renderer::~Renderer()
{
	// destroy shader first, because it may use OpenGL context
	if (shader)
		delete shader;
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

bool Renderer::init(const vector<string>& shaderFilepaths)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(1200, 720, "LearnOpenGL", nullptr, nullptr);
	if(window == nullptr)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		cout << "Failed to initialize GLAD" << endl;
		return false;
	}

	stbi_set_flip_vertically_on_load(true);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1200, 720);

	shader = new Shader();
	if(!shader->loadShaders(shaderFilepaths))
	{
		cout << "Failed to load shaders" << endl;
		return false;
	}

	return true;
}

void Renderer::run()
{
	assert(window != nullptr && "Window is not initialized");

	LightManager lightManager;

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const auto currentTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		g_camera.update(deltaTime);

		shader->use(0);
		g_camera.send(*shader);
		lightManager.send(*shader);

		auto modelView = modelRegistry.view<Model, vector<TransformComponent>>();
		for (auto entity : modelView)
		{
			auto& modelComp = modelView.get<Model>(entity);
			auto& transforms = modelView.get<vector<TransformComponent>>(entity);
			for (const auto& transform : transforms)
			{
				mat4 model = mat4(1.0f);
				model = translate(model, transform.position);
				model = rotate(model, radians(transform.rotation.x), vec3(1.0f, 0.0f, 0.0f));
				model = rotate(model, radians(transform.rotation.y), vec3(0.0f, 1.0f, 0.0f));
				model = rotate(model, radians(transform.rotation.z), vec3(0.0f, 0.0f, 1.0f));
				model = scale(model, transform.scale);
				shader->setMat4("model", model);
				modelComp.draw(*shader);
			}
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::loadModel(const string& modelPath, const vector<TransformComponent>& transforms)
{
	const string base(DATA_DIR);
	const entt::entity model = modelRegistry.create();
	modelRegistry.emplace<Model>(model, base + "/models/" + modelPath);
	modelRegistry.emplace<vector<TransformComponent>>(model, transforms);
}
