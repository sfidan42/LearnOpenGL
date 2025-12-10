#include "Material.hpp"

Material::Material(const Texture& diffuseTex, const Texture& specularTex)
: diffuse(diffuseTex), specular(specularTex)
{
	// Currently only diffuse texture is used
}

void Material::bind() const
{
    diffuse.bind(0);
	specular.bind(1);
}

void Material::send(const Shader& shader) const
{
	shader.set1f("material.shininess", shininess);
	shader.set1i("material.diffuse", 0);
	shader.set1i("material.specular", 1);
}
