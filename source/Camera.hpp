#pragma once
#include <glm/glm.hpp>

#include "Shader.hpp"

class Camera
{
public:
	glm::vec3 speed{0.0f, 0.0f, 0.0f};

	void setAspect(int width, int height);

	void mouse(float xoffset, float yoffset);
	void update(float deltaTime);

	void send(const Shader& shader);

private:
	glm::mat4 getView() const;
	glm::mat4 getProj() const;
	glm::vec3 getPosition() const { return eye; }

	// motion
	glm::vec3 eye{0.0f, 0.0f, 3.0f};
	glm::vec3 target{0.0f, 0.0f, 0.0f};
	glm::vec3 up{0.0f, 1.0f, 0.0f};

	// render
	float aspect{800.0f / 600.0f};
	float fov{45.0f};
	float zNear{0.1f};
	float zFar{100.0f};

	// direction
	float yaw{-90.0f};
	float pitch{0.0f};
	float sensitivity{0.1f};
};
