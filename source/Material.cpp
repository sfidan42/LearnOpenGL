#include "Material.hpp"

void Material::send(const Shader& shader) const
{
	shader.set1f("material.shininess", shininess);
	shader.set3f("material.ambient", ambient.r, ambient.g, ambient.b);
	shader.set3f("material.diffuse", diffuse.r, diffuse.g, diffuse.b);
	shader.set3f("material.specular", specular.r, specular.g, specular.b);
}
