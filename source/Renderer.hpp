#pragma once
#include <SDL3/SDL.h>
#include "Camera.hpp"
#include "Shader.hpp"
#include "Components.hpp"
#include "Light.hpp"
#include "Skybox.hpp"

class Renderer
{
public:
	Renderer() = default;
	~Renderer();

	bool init(SDL_Window* sdlWindow);
	void event(const SDL_Event& event);
	void render(float deltaTime);

	entt::entity loadModel(const string& modelPath, const TransformComponent& transform);

	LightManager& getLightManager() const { return *lightManager; }

private:
	void renderShadowPass();
	void renderPointLightShadows();
	void renderSpotlightShadows();
	void renderScene();

	SDL_Window* window = nullptr;
	SDL_GLContext glContext = nullptr;
	int windowWidth = 1200;
	int windowHeight = 720;

	enum shaderType
	{
		MAIN_SHADER,
		SKYBOX_SHADER,
		SHADOW_MAP_SHADER,
		SHADOW_POINT_SHADER,
		NUM_SHADERS,
	};

	string shaderFiles[NUM_SHADERS] = {
		"shaders/main.glsl",
		"shaders/skybox.glsl",
		"shaders/shadow_map.glsl",
		"shaders/shadow_point.glsl",
	};

	vector<Shader> shaders;

	Camera camera;

	Skybox* skybox = nullptr;

	LightManager* lightManager = nullptr;

	entt::registry modelRegistry;

	bool isFocused = false;
};
