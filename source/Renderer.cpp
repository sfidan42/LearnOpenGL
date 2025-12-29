#include "Renderer.hpp"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include "callbacks.hpp"
#include <stb_image.h>
#include <iostream>
#include <algorithm>
#include "Light.hpp"

Renderer::~Renderer()
{
	// destroy shader first, because it may use OpenGL context
	delete shader;
	if(window)
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

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1200, 720);

	stbi_set_flip_vertically_on_load(true);

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

		auto renderView = modelRegistry.view<InstanceComponent, TransformComponent>();
		glDepthMask(GL_TRUE);
		for(auto e : renderView)
		{
			auto& [modelEntity] = renderView.get<InstanceComponent>(e);
			auto& [positionVec, rotationVec, scaleVec] = renderView.get<TransformComponent>(e);
			auto& [path, model] = modelRegistry.get<ModelComponent>(modelEntity);

			mat4 modelMat(1.0f);
			modelMat = translate(modelMat, positionVec);
			modelMat = rotate(modelMat, rotationVec.x, {1, 0, 0});
			modelMat = rotate(modelMat, rotationVec.y, {0, 1, 0});
			modelMat = rotate(modelMat, rotationVec.z, {0, 0, 1});
			modelMat = scale(modelMat, scaleVec);

			shader->setFloat("u_Alpha", 1.0f);
			shader->setMat4("model", modelMat);
			model.draw(*shader);
		}

		glDepthMask(GL_TRUE);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::loadModel(const std::string& modelPath, const TransformComponent& transform)
{
	entt::entity modelEntity = entt::null;

	// 1. Find or create the model resource entity
	auto modelView = modelRegistry.view<ModelComponent>();
	for(auto e : modelView)
	{
		if(modelView.get<ModelComponent>(e).path == modelPath)
		{
			modelEntity = e;
			break;
		}
	}

	if(modelEntity == entt::null)
	{
		const std::string base(DATA_DIR);
		const std::string fullPath = base + "/models/" + modelPath;

		modelEntity = modelRegistry.create();
		modelRegistry.emplace<ModelComponent>(
			modelEntity,
			modelPath,
			Model(fullPath)
		);
	}

	// 2. Create an instance entity
	const entt::entity instance = modelRegistry.create();
	modelRegistry.emplace<InstanceComponent>(instance, modelEntity);
	modelRegistry.emplace<TransformComponent>(instance, transform);
}
