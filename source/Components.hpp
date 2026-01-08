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

void setupInstanceTracking(entt::registry& registry);

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
	uint64_t cubeMapHandle; // Bindless cube map handle (8 bytes)
	float _pad[2] = { 0.0f, 0.0f }; // Padding to 16-byte alignment
}; // Total 80 bytes

struct SpotlightComponent
{
	vec3 position;
	float cutOff; // Row 1: 16 bytes
	vec3 direction;
	float outerCutOff; // Row 2: 16 bytes
	vec3 ambient;
	float constant; // Row 3: 16 bytes
	vec3 diffuse;
	float linear; // Row 4: 16 bytes
	vec3 specular;
	float quadratic; // Row 5: 16 bytes
	mat4 lightSpaceMatrix; // Rows 6-9: 64 bytes
	uint64_t shadowMapHandle; // Bindless 2D shadow map handle (8 bytes)
	float _pad[2] = { 0.0f, 0.0f }; // Padding to 16-byte alignment
}; // Total 160 bytes

struct DirLightComponent
{
	vec3 direction;
	float _pad0 = 0.0f; // Row 1: 16 bytes
	vec3 ambient;
	float _pad1 = 0.0f; // Row 2: 16 bytes
	vec3 diffuse;
	float _pad2 = 0.0f; // Row 3: 16 bytes
	vec3 specular;
	float _pad3 = 0.0f; // Row 4: 16 bytes
	mat4 lightSpaceMatrix; // Rows 5-8: 64 bytes
	uint64_t shadowMapHandle; // Bindless 2D shadow map handle (8 bytes)
	float _pad4[2] = { 0.0f, 0.0f }; // Padding to 16-byte alignment
}; // Total 144 bytes

struct PointShadowMapComponent
{
	GLuint frameBuffer;
	GLuint depthCubeMap;
	uint32_t shadowSize;
};

struct SpotShadowMapComponent
{
	GLuint frameBuffer;
	GLuint depthTexture;
	uint32_t shadowWidth;
	uint32_t shadowHeight;
};

struct DirShadowMapComponent
{
	GLuint frameBuffer;
	GLuint depthTexture;
	uint32_t shadowWidth;
	uint32_t shadowHeight;
};
