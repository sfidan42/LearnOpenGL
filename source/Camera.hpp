#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
	Camera() = default;
	~Camera() = default;

	glm::mat4 getView() const;
	glm::mat4 getProj() const;

private:
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
	float aspect;
	float fov;
	float zNear;
	float zFar;
};
