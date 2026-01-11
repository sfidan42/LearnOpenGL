#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

Camera::Camera(const Shader& mainShader, const Shader& skyShader)
: cachedMainShader(mainShader), cachedSkyShader(skyShader)
{}

void Camera::setAspect(const float width, const float height)
{
	aspect = width / height;
}

void Camera::mouse(float const xoffset, float const yoffset)
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

void Camera::sync() const
{
	const mat4 proj = getProj();
	const mat4 view = getView();

	cachedMainShader.use();
	cachedMainShader.setMat4("projection", proj);
	cachedMainShader.setMat4("view", view);
	cachedMainShader.setVec3("viewPos", eye);

	cachedSkyShader.use();
	cachedSkyShader.setMat4("projection", proj);
	cachedSkyShader.setMat4("view", mat4(mat3(view))); // Remove translation
}

mat4 Camera::getView() const
{
	return lookAt(eye, target, up);
}

mat4 Camera::getProj() const
{
	return perspective(radians(fov), aspect, zNear, zFar);
}
