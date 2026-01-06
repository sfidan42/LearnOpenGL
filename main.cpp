#include "Renderer.hpp"
#include <glm/ext.hpp>
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include <vector>

using namespace glm;

struct AppState
{
	SDL_Window* window = nullptr;
	Renderer renderer;
	bool initialized = false;
};

static void setupScene(Renderer& renderer)
{
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
		float angle = 20.0f * i; // degrees
		auto axis = vec3(1.0f, 0.3f, 0.5f);
		axis = normalize(axis);
		quat q = angleAxis(radians(angle), axis);
		backpackTransforms[i].rotation = eulerAngles(q);
	}

	const vector tilesTransforms = {
		TransformComponent{vec3(0.0f, -1.0f, 0.0f), vec3(0.0f), vec3(0.5f)}
	};

	for(const TransformComponent& transform : backpackTransforms)
		renderer.loadModel("backpack/backpack.obj", transform);

	for(const TransformComponent& transform : tilesTransforms)
		renderer.loadModel("interior_tiles_1k.glb", transform);

	const PointLight pLight = renderer.lightManager->createPointLight(
		vec3(8.0f, 1.0f, 8.0f), // position
		vec3(1.0f, 1.0f, 1.0f) // color
	);
	pLight.lightData.diffuse.r = 0.0f;
	pLight.lightData.specular.r = 0.0f;
	renderer.lightManager->updatePointLight(pLight);

	constexpr vec3 spotPositions[] = {
		vec3(-2.0f, 5.0f, -2.0f),
		vec3(-2.0f, 5.0f, 2.0f),
		vec3(2.0f, 5.0f, -2.0f)
	};
	constexpr vec3 spotColors[] = {
		vec3(1.0f, 0.0f, 0.0f), // Red
		vec3(0.0f, 1.0f, 0.0f), // Green
		vec3(0.0f, 0.0f, 1.0f)  // Blue
	};
	for(int i = 0; i < 3; ++i)
	{
		renderer.lightManager->createSpotlight(
			spotPositions[i],
			normalize(vec3(0, -1.0f, 0)),
			spotColors[i]
		);
	}

	TransformComponent boxTransform{
		.position = vec3(18.0f, -1.0f, 18.0f),
		.rotation = vec3(0.0f),
		.scale = vec3(0.05f)
	};
	renderer.loadModel("Cardboard_Box.fbx", boxTransform);
	boxTransform.scale *= 0.3f;
	boxTransform.position *= 0.3f;
	renderer.loadModel("Cardboard_Box.fbx", boxTransform);
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

	setupScene(state->renderer);
	state->initialized = true;

	*appstate = state;
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	AppState* state = static_cast<AppState*>(appstate);

	if(!state || !state->initialized)
		return SDL_APP_FAILURE;

	state->renderer.iterate();
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if(!appstate || !event)
		return SDL_APP_FAILURE;

	AppState* state = static_cast<AppState*>(appstate);

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
		state->renderer.handleEvent(*event);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	AppState* state = static_cast<AppState*>(appstate);

	if(state)
	{
		// Renderer destructor handles OpenGL cleanup, window is destroyed here
		SDL_DestroyWindow(state->window);
		state->initialized = false;
		delete state;
	}
}
