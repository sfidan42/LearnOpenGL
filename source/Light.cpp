#include "Light.hpp"
#include <GLFW/glfw3.h>
#include <cmath>

void Light::send(const Shader& shader) const
{
	shader.set3f("light.position", position.r, position.g, position.b);
	shader.set3f("light.ambient", ambient.r, ambient.g, ambient.b);
	shader.set3f("light.diffuse", diffuse.r, diffuse.g, diffuse.b);
	shader.set3f("light.specular", specular.r, specular.g, specular.b);
}
