#pragma once
#include <string>
#include <glm/glm.hpp>

using namespace std;

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
};

struct Texture
{
	unsigned int id;
	string type;
	string path;
};

struct Material
{
	vector<Texture> textures;
};

inline Material* addMaterial(vector<Material*>& materials, Material* inMat)
{
	auto same = [](const Material* a, const Material* b) -> bool
	{
		if(a->textures.size() != b->textures.size())
			return false;
		for(int i = 0; i < a->textures.size(); i++)
			if(a->textures[i].id != b->textures[i].id)
				return false;
		return true;
	};

	for(Material* material : materials)
		if(same(material, inMat))
			return material;
	materials.push_back(inMat);
	return materials.back();
}

class Mesh
{
public:
	// mesh data
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	const Material* material;

	Mesh(const vector<Vertex>& vertices, const vector<unsigned int>& indices, const Material* material)
	{
		this->vertices = vertices;
		this->indices = indices;
		this->material = material;

		assert(material != nullptr && "Mesh material pointer is null");

		setupMesh();
	}

	void draw(const Shader& shader)
	{
		// Bind multiple diffuse textures into sampler array u_diffuseTextures and set count
		const unsigned int MAX_DIFFUSE = 16; // must match shader array size
		unsigned int texUnit = 0;
		unsigned int diffuseCount = 0;

		for(int i = 0; i < static_cast<int>(material->textures.size()); i++)
		{
			const string& name = material->textures[i].type;
			if(name == "diffuse" && diffuseCount < MAX_DIFFUSE)
			{
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_2D, material->textures[i].id);
				// set sampler for array element
				shader.set1i(string("u_diffuseTextures[") + to_string(diffuseCount) + string("]"), texUnit);
				++diffuseCount;
				++texUnit;
			}
			else if(name == "specular")
			{
				// bind specular to a single sampler if present (backwards-compatible)
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_2D, material->textures[i].id);
				shader.set1i("u_specular", texUnit);
				++texUnit;
			}
			else
			{
				// For any other texture types, bind them but don't set specific uniforms by default
				glActiveTexture(GL_TEXTURE0 + texUnit);
				glBindTexture(GL_TEXTURE_2D, material->textures[i].id);
				++texUnit;
			}
		}

		// tell shader how many diffuse textures are bound
		shader.set1i("u_numDiffuseTextures", static_cast<int>(diffuseCount));

		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, static_cast<int>(indices.size()), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}

private:
	//  render data
	unsigned int VAO, VBO, EBO;

	void setupMesh()
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
					 &indices[0], GL_STATIC_DRAW);

		// vertex positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

		glBindVertexArray(0);
	}
};
