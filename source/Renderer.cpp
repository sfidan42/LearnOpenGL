#include "Renderer.hpp"
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Camera.hpp"
#include <stb_image.h>
#include <iostream>

Renderer::~Renderer()
{
	// destroy these first, because they use OpenGL context
	modelRegistry.clear();
	delete lightManager;
	delete mainShader;
	delete skyboxShader;
	delete shadowMapShader;
	delete shadowPointShader;
	delete skybox;
	if(glContext)
		SDL_GL_DestroyContext(glContext);
	// Note: SDL_Window is owned by AppState in main.cpp, not by Renderer
}

bool Renderer::init(SDL_Window* sdlWindow)
{
	window = sdlWindow;

	// Set OpenGL attributes before creating context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // MSAA

	glContext = SDL_GL_CreateContext(window);
	if(!glContext)
	{
		cout << "Failed to create OpenGL context: " << SDL_GetError() << endl;
		return false;
	}

	SDL_GL_MakeCurrent(window, glContext);
	// Mouse capture is now controlled by focus mode (double-click to focus)

	if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)))
	{
		cout << "Failed to initialize GLAD" << endl;
		return false;
	}

	const GLubyte* vendor = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	const GLubyte* sl = glGetString(GL_SHADING_LANGUAGE_VERSION);

	std::cout << "OpenGL Vendor   : " << vendor << '\n';
	std::cout << "OpenGL Renderer : " << renderer << '\n';
	std::cout << "OpenGL Version  : " << version << '\n';
	std::cout << "GLSL Version    : " << sl << '\n';

	if(!GLAD_GL_ARB_bindless_texture)
	{
		cout << "Bindless textures not supported by the GPU/driver" << endl;
		return false;
	}

	// Get window size
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, windowWidth, windowHeight);
	camera.setAspect(windowWidth, windowHeight);

	glEnable(GL_CULL_FACE);

	// My redundant MSAA enable call
	glEnable(GL_MULTISAMPLE);

	const string mainShaderPath = "shaders/main.glsl";
	const string skyboxShaderPath = "shaders/skybox.glsl";

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

	// Load shadow map shader
	shadowMapShader = new Shader();
	if(!shadowMapShader->load("shaders/shadow_map.glsl"))
	{
		cout << "Failed to load shadow shaders" << endl;
		return false;
	}

	// Load point light shadow shader (with geometry shader for cube map)
	shadowPointShader = new Shader();
	if(!shadowPointShader->load("shaders/shadow_point.glsl"))
	{
		cout << "Failed to load point light shadow shaders" << endl;
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

	lightManager = new LightManager(*mainShader, *skyboxShader);

	return true;
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
			camera.setAspect(static_cast<float>(windowWidth), static_cast<float>(windowHeight));
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
						if(isFocused) camera.speed.z += 1.0f;
						break;
					case SDL_SCANCODE_S:
						if(isFocused) camera.speed.z -= 1.0f;
						break;
					case SDL_SCANCODE_A:
						if(isFocused) camera.speed.x -= 1.0f;
						break;
					case SDL_SCANCODE_D:
						if(isFocused) camera.speed.x += 1.0f;
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
					case SDL_SCANCODE_W: camera.speed.z -= 1.0f;
						break;
					case SDL_SCANCODE_S: camera.speed.z += 1.0f;
						break;
					case SDL_SCANCODE_A: camera.speed.x += 1.0f;
						break;
					case SDL_SCANCODE_D: camera.speed.x -= 1.0f;
						break;
					default: break;
				}
			}
			break;

		case SDL_EVENT_MOUSE_MOTION:
			// Only process mouse motion when focused
			if(isFocused)
			{
				camera.mouse(event.motion.xrel, -event.motion.yrel);
			}
			break;

		default:
			break;
	}
}

void Renderer::render(const float deltaTime)
{
	camera.update(deltaTime);

	// ========== PASS 1: Shadow Maps ==========
	renderShadowPass(); // Directional light (2D)
	renderPointLightShadows(); // Point lights (cube maps)
	renderSpotlightShadows(); // Spotlights (2D)

	// ========== PASS 2: Main Scene ==========
	renderScene();

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

void Renderer::renderShadowPass()
{
	auto& lightRegistry = lightManager->getLightRegistry();
	if(lightRegistry.storage<DirLightShadowMap>().empty())
		return;

	// Use back-face culling so front faces are rendered to shadow map
	// This properly shadows objects below surfaces (floors, roofs, etc.)
	glCullFace(GL_BACK);

	// Enable polygon offset to reduce shadow acne and light leakage
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	shadowMapShader->use();

	// Render shadow map for each directional light
	auto view = lightRegistry.view<DirLightComponent, DirLightShadowMap>();
	for(auto [entity, light, dirShadowMap] : view.each())
	{
		dirShadowMap.bindForWriting();

		// Use the lightSpaceMatrix from the component (updated in updateDirLightMatrices)
		shadowMapShader->setMat4("lightSpaceMatrix", light.lightSpaceMatrix);

		// Render all models to shadow map
		const auto modelView = modelRegistry.view<ModelComponent>();
		modelView.each([&](const ModelComponent& modelComp)
		{
			modelComp.drawInstanced(*shadowMapShader);
		});
	}

	// Disable polygon offset
	glDisable(GL_POLYGON_OFFSET_FILL);

	// Unbind shadow framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderPointLightShadows()
{
	auto& lightRegistry = lightManager->getLightRegistry();
	if(lightRegistry.storage<PointLightShadowMap>().empty())
		return;

	// Use back-face culling so front faces are rendered to shadow map
	glCullFace(GL_BACK);

	// Enable polygon offset to reduce light leakage
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	shadowPointShader->use();

	constexpr float farPlane = PointLightShadowMap::FAR_PLANE;
	shadowPointShader->setFloat("farPlane", farPlane);

	// Get point lights to access positions
	auto view = lightRegistry.view<PointLightComponent>();
	for(auto [entity, light] : view.each())
	{
		auto* pShadowMap = lightRegistry.try_get<PointLightShadowMap>(entity);
		if(!pShadowMap)
			continue;
		pShadowMap->bindForWriting();

		shadowPointShader->setVec3("lightPos", light.position);

		// Calculate and set the 6 shadow matrices
		mat4 projection = PointLightShadowMap::getLightProjectionMatrix(0.1f, farPlane);
		auto viewMatrices = PointLightShadowMap::getLightViewMatrices(light.position);

		for(int face = 0; face < 6; ++face)
		{
			mat4 lightShadowMatrix = projection * viewMatrices[face];
			string uniformName = "shadowMatrices[" + std::to_string(face) + "]";
			shadowPointShader->setMat4(uniformName, lightShadowMatrix);
		}

		// Render all models
		const auto modelView = modelRegistry.view<ModelComponent>();
		modelView.each([this](const ModelComponent& modelComp)
		{
			modelComp.drawInstanced(*shadowPointShader);
		});
	}

	// Disable polygon offset
	glDisable(GL_POLYGON_OFFSET_FILL);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderSpotlightShadows()
{
	auto& lightRegistry = lightManager->getLightRegistry();
	if(lightRegistry.storage<SpotlightShadowMap>().empty())
		return;

	// Use back-face culling so front faces are rendered to shadow map
	glCullFace(GL_BACK);

	// Enable polygon offset to reduce light leakage
	// Higher values needed for spotlights to prevent leakage through thin geometry
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(4.0f, 8.0f);

	shadowMapShader->use();

	// Get Spotlights to access positions and directions
	auto view = lightRegistry.view<SpotlightComponent>();
	for(auto [entity, light] : view.each())
	{
		auto* sShadowMap = lightRegistry.try_get<SpotlightShadowMap>(entity);
		if(!sShadowMap)
			continue;

		sShadowMap->bindForWriting();

		mat4 lightSpaceMatrix = sShadowMap->getLightSpaceMatrix(
			light.position,
			normalize(light.direction),
			light.outerCutOff,
			0.1f, 50.0f
		);
		shadowMapShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

		// Render all models
		const auto modelView = modelRegistry.view<ModelComponent>();
		modelView.each([this](const ModelComponent& modelComp)
		{
			modelComp.drawInstanced(*shadowMapShader);
		});
	}

	// Disable polygon offset
	glDisable(GL_POLYGON_OFFSET_FILL);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::renderScene()
{
	// Restore viewport to window size
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera.send(*mainShader, *skyboxShader);

	mainShader->use();

	// All shadow maps (directional, point, and spotlight) are now handled via bindless textures
	// stored directly in the light SSBOs - no need to bind them here

	const auto modelView = modelRegistry.view<ModelComponent>();
	modelView.each([&](const ModelComponent& modelComp)
	{
		modelComp.drawInstanced(*mainShader);
	});

	skybox->draw(*skyboxShader);
}
