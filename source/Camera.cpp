#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

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

void Camera::mouse(float xoffset, float yoffset)
{
	yaw += xoffset * sensitivity;
	pitch += yoffset * sensitivity;
	pitch = glm::clamp(pitch, -89.99f, 89.99f);
}

void Camera::update(float deltaTime)
{
	const glm::vec3 front = glm::normalize(glm::vec3(
		cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
		sin(glm::radians(pitch)),
		sin(glm::radians(yaw)) * cos(glm::radians(pitch))
	));

	const glm::vec3 right = glm::normalize(glm::cross(front, up));

	eye += right * speed.x * deltaTime * 10.0f;
	eye += up * speed.y * deltaTime * 10.0f;
	eye += front * speed.z * deltaTime * 10.0f;

	target = eye + front;
}
void Camera::send(const Shader& shader)
{
	glm::mat4 proj = getProj();
	glm::mat4 view = getView();
	shader.setMat4("projection", proj);
	shader.setMat4("view", view);
	shader.setVec3("viewPos", eye);
}
