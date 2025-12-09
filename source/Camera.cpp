#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::getView() const
{
	return glm::lookAt(eye, center, up);
}

glm::mat4 Camera::getProj() const
{
	return glm::perspective(glm::radians(fov), aspect, zNear, zFar);
}
