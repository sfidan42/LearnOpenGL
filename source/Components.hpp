#pragma once
#include <string>
#include <array>
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
	float constant;
    
	vec3 ambient;
	float linear;
    
	vec3 diffuse;
	float quadratic;
    
	vec3 specular;
	float farPlane;
    
	std::array<mat4, 6> shadowMatrices;
	GLuint64 cubeMapHandle; // Bindless handle to samplerCube
	float _pad[3];
};

struct SpotlightComponent
{
	vec3 position;
	float cutOff;
    
	vec3 direction;
	float outerCutOff;
    
	vec3 ambient;
	float constant;
    
	vec3 diffuse;
	float linear;
    
	vec3 specular;
	float quadratic;
    
	mat4 lightSpaceMatrix;
	GLuint64 shadowMapHandle; // Bindless handle to sampler2DShadow
	float _pad[2];
};

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
    
	mat4 lightSpaceMatrix;
	GLuint64 shadowMapHandle; // Bindless handle to sampler2DShadow
	float _pad4[2];
};
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
