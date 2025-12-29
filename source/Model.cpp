#include "Model.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stb_image.h>
#include "Components.hpp"

void Mesh::setup(const vector<Vertex>& vertices, const vector<Index>& indices,
				 const vector<TextureComponent>& textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
				 indices.data(), GL_STATIC_DRAW);

	GLuint next = Vertex::vertexAttributes();

	glBindVertexArray(0);

	// 1. Generate the Instance VBO
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

	// We bind the VAO to attach the new attributes to it
	glBindVertexArray(VAO);

	for (unsigned int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(next + i);
		glVertexAttribPointer(next + i, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(sizeof(vec4) * i));
		// Tell OpenGL this attribute advances per instance, not per vertex
		glVertexAttribDivisor(next + i, 1);
	}

	glBindVertexArray(0);
}

void Mesh::drawInstanced(const Shader& shader, const vector<mat4>& matrices) const
{
	// Upload the gathered matrices to the instanceVBO
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(mat4), matrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	bind(shader);

	int instanceCount = static_cast<int>(matrices.size());

	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, static_cast<int>(indices.size()), GL_UNSIGNED_INT, 0, instanceCount);
	glBindVertexArray(0);
}

void Mesh::bind(const Shader& shader) const
{
	// Bind multiple diffuse textures into sampler array u_diffuseTextures and set count
	const unsigned int MAX_DIFFUSE = 16; // must match shader array size
	int texUnit = 0;
	int diffuseCount = 0;
	int specularCount = 0;

	for(const auto& texture : textures)
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
}

bool ProcessTexture(unsigned char* data, int width, int height, int nrComponents, GLuint& textureID)
{
	// Choose proper internal format and data format based on number of components
	GLint format = GL_RGB;
	switch(nrComponents)
	{
		case 1: format = GL_RED;
			break;
		case 2: format = GL_RG;
			break;
		case 3: format = GL_RGB;
			break;
		case 4: format = GL_RGBA;
			break;
		default: format = (-1);
			break; // Defensive default
	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	if(format == -1)
		cout << "Unsupported number of image components while processing the texture: " << nrComponents <<
			endl;
	else
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return format != -1;
}

unsigned int TextureFromFile(const char* path, const string& directory)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID = 0;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if(data)
	{
		if(!ProcessTexture(data, width, height, nrComponents, textureID))
			cout << "Failed to process texture at path: " << path << endl;
		stbi_image_free(data);
	}
	else
	{
		cout << "TextureComponent failed to load at path: " << path << endl;
		stbi_image_free(data);
	}

	return textureID;
}

Model::Model(const string& modelPath)
{
	cout << "------------------Model-------------------" << endl;
	loadModel(modelPath);
	cout << "Number of meshes: " << registry.view<Mesh>().storage()->size() << endl;
	auto texturesView = registry.view<TextureComponent>();
	cout << "Number of textures loaded: " << texturesView.storage()->size() << endl;
	for(auto [ent, tex] : texturesView.each())
		cout << "\tTextureComponent ID: " << tex.id << ", Type: " << tex.type << ", Path: " << tex.path << endl;
	cout << "----------------------------------------" << endl;
}

void Model::loadModel(const string& modelPath)
{
	// read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		modelPath,
		aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_TransformUVCoords
		| aiProcess_FlipUVs
	);
	// check for errors
	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
	{
		cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
		return;
	}
	// retrieve the directory path of the filepath
	directory = modelPath.substr(0, modelPath.find_last_of('/'));

	// process ASSIMP's root node recursively
	processNode(scene->mRootNode, scene);
	cout << "Model loaded successfully from: " << modelPath << endl;
}

void Model::processNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform)
{
	// Combine the current node's transformation with the parent's transformation
	aiMatrix4x4 nodeTransform = parentTransform * node->mTransformation;

	// Process each mesh located at the current node
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene, nodeTransform);
	}

	// Recursively process each of the children nodes
	for(unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, nodeTransform);
	}
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform)
{
	// Apply the transformation to the vertices
	vector<Vertex> vertices;
	vector<unsigned int> indices; // Declare indices in the correct scope
	vector<TextureComponent> textures; // Declare textures in the correct scope

	for(unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex;
		aiVector3D transformedPosition = transform * mesh->mVertices[i];
		vertex.Position = vec3(transformedPosition.x, transformedPosition.y, transformedPosition.z);
		// normals
		if(mesh->HasNormals())
		{
			aiVector3D transformedNormal = transform * mesh->mNormals[i];
			vertex.Normal = vec3(transformedNormal.x, transformedNormal.y, transformedNormal.z);
		}
		// texture coordinates
		if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			vec2 vec;
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
			vertex.TexCoords = vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	for(unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for(unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	vector<TextureComponent> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse", scene);
	vector<TextureComponent> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specular", scene);

	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	Mesh& meshComp = registry.emplace<Mesh>(registry.create());
	meshComp.setup(vertices, indices, textures);
}

vector<TextureComponent> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const string& typeName,
													 const aiScene* scene)
{
	vector<TextureComponent> textures;
	for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// check if TextureComponent was loaded before and if so, continue to next iteration: skip loading a new TextureComponent
		bool skip = false;
		auto view = registry.view<TextureComponent>();
		for(auto [ent, tex] : view.each())
		{
			if(strcmp(tex.path.data(), str.C_Str()) == 0)
			{
				textures.push_back(tex);
				skip = true;
				// a TextureComponent with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}
		if(!skip)
		{
			// if TextureComponent hasn't been loaded already, load it
			TextureComponent texture;

			const char* texPath = str.C_Str();
			// Handle embedded textures in glTF/GLB: paths like "*0"
			if(texPath && texPath[0] == '*')
			{
				// parse index after '*'
				char* endptr = nullptr;
				const long texIndexLong = strtol(texPath + 1, &endptr, 10);
				int texIndex = static_cast<int>(texIndexLong);
				if(endptr != (texPath + 1) && scene->mTextures && texIndex >= 0 && texIndex < static_cast<int>(scene
					->
					mNumTextures))
				{
					const aiTexture* atex = scene->mTextures[texIndex];

					unsigned int textureID;
					glGenTextures(1, &textureID);
					glBindTexture(GL_TEXTURE_2D, textureID);

					int width = 0, height = 0, nrComponents = 0;
					unsigned char* data = nullptr;
					bool dataAllocated = false;

					if(atex->mHeight == 0)
					{
						// compressed image format (PNG/JPEG) inside memory
						// atex->mWidth stores size in bytes and pcData points to that memory
						unsigned int size = atex->mWidth;
						data = stbi_load_from_memory(
							static_cast<unsigned char*>(const_cast<void*>(reinterpret_cast<const void*>(atex->
								pcData))), (int)size, &width, &height, &nrComponents, 0);
					}
					else
					{
						// uncompressed RGBA32 image stored as aiTexel array
						width = static_cast<int>(atex->mWidth);
						height = static_cast<int>(atex->mHeight);
						nrComponents = 4; // aiTexel has r,g,b,a
						const size_t bufSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
						data = static_cast<unsigned char*>(malloc(bufSize));
						dataAllocated = true;
						const auto texels = reinterpret_cast<aiTexel*>(atex->pcData);
						for(size_t p = 0; p < static_cast<size_t>(width) * static_cast<size_t>(height); ++p)
						{
							data[p * 4 + 0] = texels[p].r;
							data[p * 4 + 1] = texels[p].g;
							data[p * 4 + 2] = texels[p].b;
							data[p * 4 + 3] = texels[p].a;
						}
					}

					texture.id = 0;
					if(data)
					{
						if(!ProcessTexture(data, width, height, nrComponents, textureID))
							cout << "Failed to process embedded texture at path: " << texPath << endl;
						dataAllocated ? free(data) : stbi_image_free(data);
						texture.id = textureID;
					}

					texture.type = typeName;
					texture.path = texPath;
					textures.push_back(texture);
					registry.emplace<TextureComponent>(registry.create(), texture);
					continue; // processed embedded texture, go to next
				}
				cout << "Embedded texture index out of range: " << texPath << endl;
			}

			// fallback: try loading texture from file on disk
			texture.id = TextureFromFile(str.C_Str(), this->directory);
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			registry.emplace<TextureComponent>(registry.create(), texture);
			// store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
		}
	}
	return textures;
}

Model::Model(Model&& other) noexcept
: directory(std::move(other.directory)),
  registry(std::move(other.registry)) {}

Model& Model::operator=(Model&& other) noexcept
{
	if(this != &other)
	{
		directory = std::move(other.directory);
		registry = std::move(other.registry);
	}
	return *this;
}

void Model::drawInstanced(const Shader& shader, const vector<mat4>& matrices) const
{
	const auto view = registry.view<Mesh>();
	view.each([&shader, &matrices](const Mesh& mesh) { mesh.drawInstanced(shader, matrices); });
}
