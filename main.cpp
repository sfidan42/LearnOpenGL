#include "Renderer.hpp"
#include <glm/ext.hpp>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <vector>

using namespace glm;

struct Data
{
	vector<entt::entity> pointLights;
	vector<entt::entity> spotlights;
	vector<entt::entity> dirLights;
	vector<entt::entity> modelInstances;
};

struct AppState
{
	SDL_Window* window = nullptr;
	Renderer renderer;
	Data gameData;
	bool initialized = false;
};

static void setupScene(Renderer& renderer, Data& inGameData)
{
	inGameData.pointLights.reserve(4);
	inGameData.spotlights.reserve(4);
	inGameData.dirLights.reserve(1);
	inGameData.modelInstances.reserve(16);

	entt::entity aux;

	const vector backpackPositions = {
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
	vector<TransformComponent> backpackTransforms(backpackPositions.size());
	for(uint32_t i = 0; i < backpackTransforms.size(); i++)
	{
		backpackTransforms[i].position = backpackPositions[i];
		backpackTransforms[i].scale = vec3(0.2f);
		const float angle = 20.0f * static_cast<float>(i); // degrees
		auto axis = vec3(1.0f, 0.3f, 0.5f);
		quat q = angleAxis(radians(angle), normalize(axis));
		backpackTransforms[i].rotation = eulerAngles(q);
	}

	for(const TransformComponent& transform : backpackTransforms)
	{
		aux = renderer.loadModel("backpack/backpack.obj", transform);
		inGameData.modelInstances.push_back(aux);
	}

	TransformComponent tileTransform{
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f),
		vec3(0.5f)
	};
	aux = renderer.loadModel("interior_tiles_1k.glb", tileTransform);
	inGameData.modelInstances.push_back(aux);

	LightManager& lightManager = renderer.getLightManager();

	// Create directional light (sun)
	aux = lightManager.createDirLight(
		normalize(vec3(-0.1f, -1.0f, -0.3f)), // direction
		vec3(1.0f, 1.0f, 0.9f) // warm sunlight color
	);
	inGameData.dirLights.push_back(aux);

	aux = lightManager.createPointLight(
		vec3(8.0f, 1.0f, 8.0f), // position
		vec3(1.0f, 1.0f, 1.0f) // color
	);
	inGameData.pointLights.push_back(aux);
	PointLightComponent& pLight = lightManager.getPointLight(aux);
	// Set red channel to 0 to create cyan light
	pLight.diffuse.r = 0.0f;
	pLight.specular.r = 0.0f;
	lightManager.updatePointLight(aux);

	constexpr vec3 spotPositions[] = {
		vec3(-2.0f, 5.0f, -2.0f),
		vec3(-2.0f, 5.0f, 2.0f),
		vec3(2.0f, 5.0f, -2.0f)
	};
	constexpr vec3 spotColors[] = {
		vec3(1.0f, 0.0f, 0.0f), // Red
		vec3(0.0f, 1.0f, 0.0f), // Green
		vec3(0.0f, 0.0f, 1.0f) // Blue
	};
	for(int i = 0; i < 3; ++i)
	{
		aux = lightManager.createSpotlight(
			spotPositions[i],
			normalize(vec3(0, -1.0f, 0)),
			spotColors[i]
		);
		inGameData.spotlights.push_back(aux);
	}

	TransformComponent boxTransform{
		.position = vec3(18.0f, -1.0f, 18.0f),
		.rotation = vec3(0.0f),
		.scale = vec3(0.05f)
	};
	aux = renderer.loadModel("Cardboard_Box.fbx", boxTransform);
	inGameData.modelInstances.push_back(aux);
	boxTransform.scale *= 0.3f;
	boxTransform.position *= 0.3f;
	aux = renderer.loadModel("Cardboard_Box.fbx", boxTransform);
	inGameData.modelInstances.push_back(aux);

	cout << "\n============= Scene setup complete. =============" << endl;
	cout << "Number of model instances: " << inGameData.modelInstances.size() << endl;
	cout << "Number of point lights: " << inGameData.pointLights.size() << endl;
	cout << "Number of spotlights: " << inGameData.spotlights.size() << endl;
	cout << "Number of directional lights: " << inGameData.dirLights.size() << endl;
}

static void updateScene(Renderer& renderer, const Data& gameData, const float deltaTime)
{
	LightManager& lightManager = renderer.getLightManager();
	// Animate directional light (sun) over time
	static float sunAngle = 0.0f; // in degrees
	sunAngle += 10.0f * deltaTime; // degrees per second
	if(sunAngle >= 360.0f)
		sunAngle -= 360.0f;
	const vec3 sunDirection = normalize(vec3(cos(radians(sunAngle)), -0.7f, sin(radians(sunAngle))));

	const entt::entity aux = gameData.dirLights[0];
	DirLightComponent& dirLight = lightManager.getDirLight(aux);
	dirLight.direction = sunDirection;
	lightManager.updateDirLight(aux);

	renderer.update(deltaTime);
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	// Force NVIDIA GPU on hybrid graphics systems (must be set before SDL_Init)
	setenv("__NV_PRIME_RENDER_OFFLOAD", "1", 1);
	setenv("__GLX_VENDOR_LIBRARY_NAME", "nvidia", 1);

	if(!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Failed to init SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	auto* state = new AppState();
	state->window = SDL_CreateWindow("LearnOpenGL", 1200, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if(!state->window)
	{
		SDL_Log("Failed to create window: %s", SDL_GetError());
		delete state;
		return SDL_APP_FAILURE;
	}

	SDL_SetWindowMinimumSize(state->window, 400, 300);

	if(!state->renderer.init(state->window))
	{
		SDL_Log("Failed to initialize renderer");
		SDL_DestroyWindow(state->window);
		delete state;
		return SDL_APP_FAILURE;
	}

	setupScene(state->renderer, state->gameData);
	state->initialized = true;

	*appstate = state;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	static float lastTicks = SDL_GetTicks();

	const auto state = static_cast<AppState*>(appstate);

	if(!state || !state->initialized)
		return SDL_APP_FAILURE;

	const float currentTicks = SDL_GetTicks();
	const float deltaTime = (currentTicks - lastTicks) / 1000.0f;
	lastTicks = currentTicks;

	updateScene(state->renderer, state->gameData, deltaTime);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if(!appstate || !event)
		return SDL_APP_FAILURE;

	auto* state = static_cast<AppState*>(appstate);

	switch(event->type)
	{
		case SDL_EVENT_QUIT:
			return SDL_APP_SUCCESS;
		case SDL_EVENT_KEY_DOWN:
			if(event->key.key == SDLK_ESCAPE)
				return SDL_APP_SUCCESS;
			break;
		default:
			break;
	}

	if(state->initialized)
		state->renderer.event(*event);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult /*result*/)
{
	if(!appstate)
		return;
	auto* state = static_cast<AppState*>(appstate);
	// Renderer destructor handles OpenGL cleanup, window is destroyed here
	SDL_DestroyWindow(state->window);
	state->initialized = false; // just in case
	delete state;
}
