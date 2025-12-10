#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::getView() const
{
	return glm::lookAt(eye, target, up);
}

glm::mat4 Camera::getProj() const
{
	return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
}

void Camera::setAspect(int width, int height)
{
	aspect = static_cast<float>(width) / static_cast<float>(height);
}

void Camera::update(float deltaTime)
{
	const glm::vec3 delta = speed * deltaTime * 0.01f;
	eye += delta;
	target += delta;
}
