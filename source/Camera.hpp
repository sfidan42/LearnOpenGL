#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera() : eye(0.0f, 0.0f, 3.0f), center(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f), aspect(800.0f/600.0f), fov(45.0f), zNear(0.1f), zFar(100.0f) {}
	~Camera() = default;

	glm::mat4 getView() const
	{
		return glm::lookAt(eye, center, up);
	}

	glm::mat4 getProj() const
	{
		return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
	}

private:
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
	float aspect;
	float fov;
	float zNear;
	float zFar;
};
