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

	[[nodiscard]] mat4 bake() const;
};

struct TextureComponent
{
	GLuint id;
	GLuint64 handle; // Bindless texture handle
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
	vector<entt::entity> instances;
	vector<mat4> instanceMatrices;

	void drawInstanced(const Shader& shader) const;
};

struct PointLightComponent
{
	vec3 position;
	float constant; // Row 1: 16 bytes
	vec3 ambient;
	float linear; // Row 2: 16 bytes
	vec3 diffuse;
	float quadratic; // Row 3: 16 bytes
	vec3 specular;
	float farPlane; // Row 4: 16 bytes - far plane for shadow calculation
	uint64_t shadowMapHandle; // Bindless cubemap handle (8 bytes)
	float _pad[2]; // Padding to 16-byte alignment
}; // Total 80 bytes

struct SpotlightComponent
{
	vec3 position;
	float cutOff; // Row 1
	vec3 direction;
	float outerCutOff; // Row 2
	vec3 ambient;
	float constant; // Row 3
	vec3 diffuse;
	float linear; // Row 4
	vec3 specular;
	float quadratic; // Row 5
	mat4 lightSpaceMatrix; // Rows 6-9: 64 bytes
	uint64_t shadowMapHandle; // Bindless 2D shadow map handle (8 bytes)
	float _pad[2]; // Padding to 16-byte alignment
}; // Total 160 bytes

struct DirLightComponent
{
	vec3 direction;
	float _pad0;
	vec3 ambient;
	float _pad1;
	vec3 diffuse;
	float _pad2;
	vec3 specular;
	float _pad3;
};

void setupInstanceTracking(entt::registry& registry);
