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
	modelRegistry.clear();
	delete lightManager;
	delete mainShader;
	delete skyboxShader;
	delete shadowShader;
	delete skybox;
	delete shadowMap;
	if(window)
		glfwDestroyWindow(window);
	glfwTerminate();
}

bool Renderer::init(const string& mainShaderPath, const string& skyboxShaderPath, const string& shadowShaderPath)
{
	setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1);
	setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

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

	const GLubyte* vendor   = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version  = glGetString(GL_VERSION);
	const GLubyte* sl       = glGetString(GL_SHADING_LANGUAGE_VERSION);

	std::cout << "OpenGL Vendor   : " << vendor   << '\n';
	std::cout << "OpenGL Renderer : " << renderer << '\n';
	std::cout << "OpenGL Version  : " << version  << '\n';
	std::cout << "GLSL Version    : " << sl       << '\n';

	if (!GLAD_GL_ARB_bindless_texture)
	{
		cout << "Bindless textures not supported by the GPU/driver" << endl;
		return false;
	}

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, 1200, 720);
	g_camera.setAspect(1200, 720);

	glEnable(GL_CULL_FACE);

	// My redundant MSAA enable call
	glEnable(GL_MULTISAMPLE);

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

	// Load shadow shader
	shadowShader = new Shader();
	if(!shadowShader->load(shadowShaderPath))
	{
		cout << "Failed to load shadow shaders" << endl;
		return false;
	}

	// Initialize shadow map (2048x2048 resolution)
	shadowMap = new ShadowMap(2048, 2048);

	// Set shadow map texture unit in main shader
	mainShader->use();
	mainShader->setInt("shadowMap", 5); // Use texture unit 5 for shadow map

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

	lightManager = new LightManager(*mainShader, *skyboxShader);

	return true;
}

void Renderer::run()
{
	assert(window != nullptr && "Window is not initialized");

	float lastTime = 0.0f;

	while(!glfwWindowShouldClose(window))
	{
		const auto currentTime = static_cast<float>(glfwGetTime());
		const float deltaTime = currentTime - lastTime;
		lastTime = currentTime;

		lightManager->update(deltaTime);
		g_camera.update(deltaTime);

		// ========== PASS 1: Shadow Map ==========
		renderShadowPass();

		// ========== PASS 2: Main Scene ==========
		renderScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void Renderer::renderShadowPass()
{
	shadowMap->bindForWriting();

	// Use front face culling to reduce shadow acne
	glCullFace(GL_FRONT);

	shadowShader->use();

	// Calculate light space matrix from sun direction
	const vec3 sunDir = lightManager->getSunDirection();
	const mat4 lightSpaceMatrix = shadowMap->getLightSpaceMatrix(sunDir, 30.0f, 0.1f, 100.0f);
	shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

	// Also send to main shader for fragment shader use
	mainShader->use();
	mainShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

	// Render all models to shadow map
	shadowShader->use();
	const auto modelView = modelRegistry.view<ModelComponent>();
	modelView.each([&](const ModelComponent& modelComp)
	{
		modelComp.drawInstanced(*shadowShader);
	});

	// Reset to back face culling
	glCullFace(GL_BACK);

	// Unbind shadow framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderScene()
{
	// Restore viewport to window size
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	g_camera.send(*mainShader, *skyboxShader);

	// Bind shadow map for reading
	shadowMap->bindForReading(GL_TEXTURE5);

	mainShader->use();

	const auto modelView = modelRegistry.view<ModelComponent>();
	modelView.each([&](const ModelComponent& modelComp)
	{
		modelComp.drawInstanced(*mainShader);
	});

	skybox->draw(*skyboxShader);
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
	// TODO: when scale is different thatn (1,1,1), position and rotation may need adjustment

	// 3. Bake the transform and add it to the ModelComponent's instanceMatrices
	auto& modelComp = modelRegistry.get<ModelComponent>(modelEntity);
	modelComp.instanceMatrices.emplace_back(transform.bake());
}
