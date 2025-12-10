#pragma once
#include <glm/glm.hpp>

class Camera
{
public:
	glm::vec3 speed{0.0f, 0.0f, 0.0f};

	glm::mat4 getView() const;
	glm::mat4 getProj() const;
	void setAspect(int width, int height);

	void update(float deltaTime);

private:
	glm::vec3 eye{0.0f, 0.0f, 3.0f};
	glm::vec3 target{0.0f, 0.0f, 0.0f};
	glm::vec3 up{0.0f, 1.0f, 0.0f};
	float aspect{800.0f / 600.0f};
	float fov{45.0f};
	float zNear{0.1f};
	float zFar{100.0f};
};
