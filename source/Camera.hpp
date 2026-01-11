#pragma once
#include <glm/glm.hpp>
#include "Shader.hpp"

using namespace glm;

class Camera
{
public:
	Camera(const Shader& mainShader, const Shader& skyShader);

	vec3 speed{0.0f, 0.0f, 0.0f};

	void setAspect(float width, float height);

	void mouse(float xoffset, float yoffset);
	void update(float deltaTime);

	void sync() const;

private:
	[[nodiscard]] mat4 getView() const;
	[[nodiscard]] mat4 getProj() const;

	// motion
	vec3 eye{0.0f, 0.0f, 3.0f};
	vec3 target{0.0f, 0.0f, 0.0f};
	vec3 up{0.0f, 1.0f, 0.0f};

	// render
	float aspect{800.0f / 600.0f};
	float fov{45.0f};
	float zNear{0.1f};
	float zFar{100.0f};

	// direction
	float yaw{-90.0f};
	float pitch{0.0f};
	float sensitivity{0.2f};

	// cached shaders
	const Shader& cachedMainShader;
	const Shader& cachedSkyShader;
};
