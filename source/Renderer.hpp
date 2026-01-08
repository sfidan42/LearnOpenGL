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
	Shader* mainShader = nullptr;
	Shader* skyboxShader = nullptr;
	Shader* shadowMapShader = nullptr;
	Shader* shadowPointShader = nullptr;
	Skybox* skybox = nullptr;
	LightManager* lightManager = nullptr;

	Camera camera;

	entt::registry modelRegistry;

	int windowWidth = 1200;
	int windowHeight = 720;

	float lastTime = 0.0f;
	bool isFocused = false;
};
