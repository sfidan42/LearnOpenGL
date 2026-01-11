#include "Renderer.hpp"
#include <glm/ext.hpp>
#include <glm/detail/type_quat.hpp>
#include "Camera.hpp"
#include <stb_image.h>
#include <iostream>

Renderer::~Renderer()
{
	// destroy these first, because they use OpenGL context
	modelRegistry.clear();
	shaders.clear();
	delete lightManager;
	delete camera;
	delete skybox;
	if(glContext)
		SDL_GL_DestroyContext(glContext);
	// Note: SDL_Window is owned by AppState in main.cpp, not by Renderer
}

void Renderer::init(SDL_Window* sdlWindow)
{
	window = sdlWindow;

	initOpenGL();
	initShaders();
	loadSkybox();
	initCamera();
	initLightManager();

	setupInstanceTracking(modelRegistry);
	stbi_set_flip_vertically_on_load(true);
}

void Renderer::event(const SDL_Event& event)
{
	switch(event.type)
	{
		case SDL_EVENT_WINDOW_RESIZED:
		case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
			windowWidth = event.window.data1;
			windowHeight = event.window.data2;
			glViewport(0, 0, windowWidth, windowHeight);
			if(camera)
				camera->setAspect(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			// Double-click to enter focus mode
			if(event.button.button == SDL_BUTTON_LEFT && event.button.clicks == 2)
			{
				if(!isFocused)
				{
					isFocused = true;
					SDL_SetWindowRelativeMouseMode(window, true);
				}
			}
			break;

		case SDL_EVENT_KEY_DOWN:
			if(!event.key.repeat)
			{
				switch(event.key.scancode)
				{
					case SDL_SCANCODE_Q:
						// Exit focus mode
						if(isFocused)
						{
							isFocused = false;
							SDL_SetWindowRelativeMouseMode(window, false);
						}
						break;
					case SDL_SCANCODE_W:
						if(isFocused) if(camera) camera->speed.z += 1.0f;
						break;
					case SDL_SCANCODE_S:
						if(isFocused) if(camera) camera->speed.z -= 1.0f;
						break;
					case SDL_SCANCODE_A:
						if(isFocused) if(camera) camera->speed.x -= 1.0f;
						break;
					case SDL_SCANCODE_D:
						if(isFocused) if(camera) camera->speed.x += 1.0f;
						break;
					default: break;
				}
			}
			break;

		case SDL_EVENT_KEY_UP:
			if(isFocused)
			{
				switch(event.key.scancode)
				{
					case SDL_SCANCODE_W: if(camera) camera->speed.z -= 1.0f;
						break;
					case SDL_SCANCODE_S: if(camera) camera->speed.z += 1.0f;
						break;
					case SDL_SCANCODE_A: if(camera) camera->speed.x += 1.0f;
						break;
					case SDL_SCANCODE_D: if(camera) camera->speed.x -= 1.0f;
						break;
					default: break;
				}
			}
			break;

		case SDL_EVENT_MOUSE_MOTION:
			// Only process mouse motion when focused
			if(isFocused)
			{
				if(camera) camera->mouse(event.motion.xrel, -event.motion.yrel);
			}
			break;

		default:
			break;
	}
}

void Renderer::update(const float deltaTime)
{
	camera->update(deltaTime);

	auto drawModels = [this](const Shader& shader)
	{
		const auto modelView = modelRegistry.view<ModelComponent>();
		modelView.each([&shader](const ModelComponent& modelComp)
		{
			modelComp.drawInstanced(shader);
		});
	};

	// ========== PASS 1: Shadow Maps ==========
	lightManager->renderShadows(drawModels);

	// ========== PASS 2: Main Scene ==========
	renderScene(drawModels);

	SDL_GL_SwapWindow(window);
}

entt::entity Renderer::loadModel(const string& modelPath, const TransformComponent& transform)
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
	cout << "Instance created for model: " << modelPath << endl;
	// TODO: when scale is different than (1,1,1), position and rotation may need adjustment

	// 3. Bake the transform and add it to the ModelComponent's instanceMatrices
	auto& modelComp = modelRegistry.get<ModelComponent>(modelEntity);
	modelComp.instanceMatrices.emplace_back(transform.bake());

	return instance;
}

void Renderer::initOpenGL()
{
	// Set OpenGL attributes before creating context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // MSAA

	glContext = SDL_GL_CreateContext(window);
	if(!glContext)
		throw std::runtime_error("Failed to create OpenGL context: " + string(SDL_GetError()));

	SDL_GL_MakeCurrent(window, glContext);

	if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
		throw std::runtime_error("Failed to initialize GLAD");

	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* sl = glGetString(GL_SHADING_LANGUAGE_VERSION);

	std::cout << "OpenGL Vendor   : " << vendor << '\n';
	std::cout << "OpenGL Renderer : " << renderer << '\n';
	std::cout << "OpenGL Version  : " << version << '\n';
	std::cout << "GLSL Version    : " << sl << '\n';

	if(!GLAD_GL_ARB_bindless_texture)
		throw std::runtime_error("Bindless textures not supported by the OpenGL driver");

	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	glViewport(0, 0, windowWidth, windowHeight);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
}

void Renderer::initShaders()
{
	shaders.clear();
	shaders.reserve(NUM_SHADERS);
	for(int type = 0; type < NUM_SHADERS; ++type)
	{
		cout << "Loading shader: " << shaderFiles[type] << endl;
		shaders.emplace_back(shaderFiles[type]);
		if(!shaders[type].ok())
			throw std::runtime_error("Failed to load shader: " + shaderFiles[type]);
	}
}

void Renderer::loadSkybox()
{
	skybox = new Skybox(shaders[SKYBOX_SHADER]);

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
}

void Renderer::initCamera()
{
	camera = new Camera(shaders[MAIN_SHADER], shaders[SKYBOX_SHADER]);
	camera->setAspect(windowWidth, windowHeight);
}

void Renderer::initLightManager()
{
	lightManager = new LightManager(
		shaders[MAIN_SHADER],
		shaders[SKYBOX_SHADER],
		shaders[SHADOW_MAP_SHADER],
		shaders[SHADOW_POINT_SHADER]
	);
}

void Renderer::renderScene(const DrawModelsCallback& drawModels) const
{
	glDisable(GL_CULL_FACE);
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera->sync();

	const Shader& mainShader = shaders[MAIN_SHADER];

	mainShader.use();
	drawModels(mainShader);
	skybox->draw();

	glEnable(GL_CULL_FACE);
}
