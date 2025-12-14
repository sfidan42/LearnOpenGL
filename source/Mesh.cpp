#include "Mesh.hpp"

Mesh::Mesh(const vector<Vertex>& vertices, const vector<unsigned int>& indices, const Material* material)
{
	this->vertices = vertices;
	this->indices = indices;
	this->material = material;

	assert(material != nullptr && "Mesh material pointer is null");

	setupMesh();
}

Mesh::~Mesh()
{
	// TODO: properly mesh data
}

void Mesh::draw(const Shader& shader) const
{
	// Bind multiple diffuse textures into sampler array u_diffuseTextures and set count
	const unsigned int MAX_DIFFUSE = 16; // must match shader array size
	int texUnit = 0;
	int diffuseCount = 0;
	int specularCount = 0;

	for(const auto& texture : material->textures)
	{
		const string& name = texture.type;
		if(name == "diffuse" && diffuseCount < MAX_DIFFUSE)
		{
			glActiveTexture(GL_TEXTURE0 + texUnit);
			glBindTexture(GL_TEXTURE_2D, texture.id);
			// set sampler for array element
			shader.setInt(string("u_diffuseTextures[") + to_string(diffuseCount) + string("]"), texUnit);
			++diffuseCount;
			++texUnit;
		}
		else if(name == "specular")
		{
			// bind specular to a single sampler if present (backwards-compatible)
			glActiveTexture(GL_TEXTURE0 + texUnit);
			glBindTexture(GL_TEXTURE_2D, texture.id);
			shader.setInt(string("u_specularTextures[") + to_string(specularCount) + string("]"), texUnit);
			++specularCount;
			++texUnit;
		}
		else
		{
			// For any other texture types, bind them but don't set specific uniforms by default
			glActiveTexture(GL_TEXTURE0 + texUnit);
			glBindTexture(GL_TEXTURE_2D, texture.id);
			++texUnit;
		}
	}

	// tell shader how many diffuse textures are bound
	shader.setInt("u_numDiffuseTextures", diffuseCount);
	shader.setInt("u_numSpecularTextures", specularCount);

	// draw mesh
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, static_cast<int>(indices.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	// always good practice to set everything back to defaults once configured.
	glActiveTexture(GL_TEXTURE0);
}

void Mesh::setupMesh()
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

	Vertex::vertexAttributes();

	glBindVertexArray(0);
}
