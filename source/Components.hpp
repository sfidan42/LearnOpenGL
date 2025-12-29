#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Model.hpp"

using namespace std;
using namespace glm;

struct TransformComponent
{
	vec3 position;
	vec3 rotation;
	vec3 scale;
};

struct TextureComponent
{
	GLuint id;
	string type;
	string path;
};

struct InstanceComponent
{
	entt::entity modelEntity;
};

struct ModelComponent
{
	string path;
	Model model;
	std::vector<entt::entity> instances;
};
