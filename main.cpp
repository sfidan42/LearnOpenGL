#include "Renderer.hpp"
#include <glm/ext.hpp>

#include "callbacks.hpp"
#include "stb_image.h"

int main()
{
	Renderer renderer;

	const string mainShaderPath = "shaders/main.glsl";
	const string skyboxShaderPath = "shaders/skybox.glsl";
	const string shadowShaderPath = "shaders/shadow.glsl";

	if(!renderer.init(mainShaderPath, skyboxShaderPath, shadowShaderPath))
	{
		cout << "Failed to initialize renderer" << endl;
		return -1;
	}

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
	renderer.lightManager->updatePointLight(pLight); // Apply changes

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
		renderer.lightManager->createSpotLight(
			spotPositions[i],
			normalize(vec3(0, -1.0f, 0)),
			spotColors[i]
		);
	}

	constexpr TransformComponent boxTransform{
		.position = vec3(18.0f, -1.0f, 18.0f),
		.rotation = vec3(0.0f),
		.scale = vec3(0.05f)
	};
	renderer.loadModel("Cardboard_Box.fbx", boxTransform);

	renderer.run();

	return 0;
}
