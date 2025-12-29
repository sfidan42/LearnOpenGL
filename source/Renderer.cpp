#include "Renderer.hpp"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include "callbacks.hpp"
#include <stb_image.h>
#include <iostream>
#include <algorithm>

Renderer::~Renderer()
{
	// destroy these first, because they use OpenGL context
	delete mainShader;
	delete skyboxShader;
	delete skybox;
	if(window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

void onInstanceAdded(entt::registry& registry, entt::entity instanceEnt)
{
	auto& inst = registry.get<InstanceComponent>(instanceEnt);

	if(!registry.valid(inst.modelEntity))
		return;

	auto& modelComp = registry.get<ModelComponent>(inst.modelEntity);

	modelComp.instances.push_back(instanceEnt);
}

void onInstanceRemoved(entt::registry& registry, entt::entity instanceEnt)
{
	// The component still exists at this point
	auto& inst = registry.get<InstanceComponent>(instanceEnt);

	if(!registry.valid(inst.modelEntity))
		return;

	auto& instances = registry.get<ModelComponent>(inst.modelEntity).instances;

	erase(instances, instanceEnt);
}

void onInstanceUpdated(entt::registry& registry, entt::entity instanceEnt)
{
	auto& inst = registry.get<InstanceComponent>(instanceEnt);

	// Previous value is available through the patch mechanism
	registry.patch<InstanceComponent>(instanceEnt, [&](auto& previous)
	{
		if(previous.modelEntity == inst.modelEntity)
			return;

		// Remove from old model
		if(registry.valid(previous.modelEntity))
		{
			auto& oldList =
				registry.get<ModelComponent>(previous.modelEntity).instances;
			erase(oldList, instanceEnt);
		}

		// Add to new model
		if(registry.valid(inst.modelEntity))
		{
			auto& newList =
				registry.get<ModelComponent>(inst.modelEntity).instances;
			newList.push_back(instanceEnt);
		}
	});
}

void setupInstanceTracking(entt::registry& registry)
{
	registry.on_construct<InstanceComponent>()
			.connect<&onInstanceAdded>();

	registry.on_destroy<InstanceComponent>()
			.connect<&onInstanceRemoved>();

	registry.on_update<InstanceComponent>()
			.connect<&onInstanceUpdated>();
}

bool Renderer::init(const string& mainShaderPath, const string& skyboxShaderPath)
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
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
	g_camera.setAspect(1200, 720);

	glEnable(GL_CULL_FACE);

	mainShader = new Shader();
	if(!mainShader->load(mainShaderPath))
	{
		cout << "Failed to load shaders" << endl;
		return false;
	}

	skyboxShader = new Shader();
	if(!skyboxShader->load(skyboxShaderPath))
	{
		cout << "Failed to load skybox shaders" << endl;
		return false;
	}

	skybox = new Skybox();

	const string faces[6] = {
		"right.jpg",
		"left.jpg",
		"top.jpg",
		"bottom.jpg",
		"front.jpg",
		"back.jpg"
	};

	const string skyboxDir = string(DATA_DIR) + "/textures/skybox";
	skybox->loadFaces(skyboxDir, faces);
	skybox->scale(1000.0f);

	stbi_set_flip_vertically_on_load(true);

	setupInstanceTracking(modelRegistry);

	return true;
}

void Renderer::run()
{
	assert(window != nullptr && "Window is not initialized");

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const auto currentTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		lightManager.update(deltaTime);
		lightManager.send(*mainShader, *skyboxShader);

		g_camera.update(deltaTime);
		g_camera.send(*mainShader, *skyboxShader);

		mainShader->use();
		for (const auto modelEnt : modelRegistry.view<ModelComponent>())
		{
			auto& modelComp = modelRegistry.get<ModelComponent>(modelEnt);
			if (modelComp.instances.empty())
				continue;

			vector<mat4> matrices(modelComp.instances.size());
			for (size_t i = 0; i < modelComp.instances.size(); ++i)
			{
				auto instanceEnt = modelComp.instances[i];
				auto& [positionVec, rotationVec, scaleVec] = modelRegistry.get<TransformComponent>(instanceEnt);

				mat4 modelMat(1.0f);
				modelMat = translate(modelMat, positionVec);
				modelMat = rotate(modelMat, rotationVec.x, {1, 0, 0});
				modelMat = rotate(modelMat, rotationVec.y, {0, 1, 0});
				modelMat = rotate(modelMat, rotationVec.z, {0, 0, 1});
				modelMat = scale(modelMat, scaleVec);

				matrices[i] = modelMat;
			}
			modelComp.model.drawInstanced(*mainShader, matrices);
		}

		skybox->draw(*skyboxShader);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::loadModel(const string& modelPath, const TransformComponent& transform)
{
	// TODO: some models need glCullFace(GL_FRONT), others GL_BACK or disabled culling
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
		const string base(DATA_DIR);
		const string fullPath = base + "/models/" + modelPath;

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
