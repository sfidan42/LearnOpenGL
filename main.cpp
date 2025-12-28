#include "Renderer.hpp"
#include <glm/ext.hpp>

int main()
{
	Renderer renderer;

	vector<string> shader_filepaths = {
		"shaders/programs.glsl",
	};

	if(!renderer.init(shader_filepaths))
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
	for (uint32_t i = 0; i < backpackTransforms.size(); i++)
	{
		backpackTransforms[i].position = backpackPositions[i];
		backpackTransforms[i].scale = vec3(0.2f);
		float angle = 20.0f * i; // degrees
		vec3 axis = vec3(1.0f, 0.3f, 0.5f);
		axis = normalize(axis);
		quat q = angleAxis(radians(angle), axis);
		backpackTransforms[i].rotation = eulerAngles(q);
	}

	const vector tilesTransforms = {
		TransformComponent{vec3(0.0f, -1.5f, 0.0f), vec3(0.0f), vec3(10.0f)}
	};

	for (const TransformComponent& transform : backpackTransforms)
		renderer.loadModel("backpack/backpack.obj", transform);

	for (const TransformComponent& transform : tilesTransforms)
		renderer.loadModel("interior_tiles_1k.glb", transform);

	renderer.run();

	return 0;
}
