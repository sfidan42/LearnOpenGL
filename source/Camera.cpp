#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

mat4 Camera::getView() const
{
	return glm::lookAt(eye, target, up);
}

mat4 Camera::getProj() const
{
	return perspective(radians(fov), aspect, zNear, zFar);
}

void Camera::setAspect(float width, float height)
{
	aspect = width / height;
}

void Camera::mouse(float xoffset, float yoffset)
{
	yaw += xoffset * sensitivity;
	pitch += yoffset * sensitivity;
	pitch = glm::clamp(pitch, -89.99f, 89.99f);
}

void Camera::update(const float deltaTime)
{
	const vec3 front = normalize(vec3(
		cos(radians(yaw)) * cos(radians(pitch)),
		sin(radians(pitch)),
		sin(radians(yaw)) * cos(radians(pitch))
	));

	const vec3 right = normalize(cross(front, up));

	eye += right * speed.x * deltaTime * 10.0f;
	eye += up * speed.y * deltaTime * 10.0f;
	eye += front * speed.z * deltaTime * 10.0f;

	target = eye + front;
}

void Camera::send(const Shader& mainShader, const Shader& skyShader) const
{
	mainShader.use();
	const mat4 proj = getProj();
	const mat4 view = getView();
	mainShader.setMat4("projection", proj);
	mainShader.setMat4("view", view);
	mainShader.setVec3("viewPos", eye);
	skyShader.use();
	skyShader.setMat4("projection", proj);
	skyShader.setMat4("view", mat4(mat3(view))); // Remove translation
}
