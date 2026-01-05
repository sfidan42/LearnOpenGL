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

	for(unsigned int i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(next + i);
		glVertexAttribPointer(next + i, 4, GL_FLOAT, GL_FALSE, sizeof(mat4), (void*)(sizeof(vec4) * i));
		// Tell OpenGL this attribute advances per instance, not per vertex
		glVertexAttribDivisor(next + i, 1);
	}

	glBindVertexArray(0);

	// === BINDLESS TEXTURE SSBO SETUP ===
	// Collect handles by texture type
	for(const auto& tex : textures)
	{
		if(tex.type == "diffuse")
			diffuseHandles.push_back(tex.handle);
		else if(tex.type == "specular")
			specularHandles.push_back(tex.handle);
		else if(tex.type == "normal")
			normalHandles.push_back(tex.handle);
	}

	// Create Diffuse Handles SSBO
	if(!diffuseHandles.empty())
	{
		glGenBuffers(1, &diffuseHandlesSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, diffuseHandlesSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 diffuseHandles.size() * sizeof(GLuint64),
					 diffuseHandles.data(),
					 GL_STATIC_DRAW);
	}

	// Create Specular Handles SSBO
	if(!specularHandles.empty())
	{
		glGenBuffers(1, &specularHandlesSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, specularHandlesSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 specularHandles.size() * sizeof(GLuint64),
					 specularHandles.data(),
					 GL_STATIC_DRAW);
	}

	// Create Normal Handles SSBO
	if(!normalHandles.empty())
	{
		glGenBuffers(1, &normalHandlesSSBO);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, normalHandlesSSBO);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 normalHandles.size() * sizeof(GLuint64),
					 normalHandles.data(),
					 GL_STATIC_DRAW);
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Mesh::drawInstanced(const Shader& shader, const vector<mat4>& instanceMatrices) const
{
	const int instanceCount = static_cast<int>(instanceMatrices.size());
	const int indexCount = static_cast<int>(indices.size());

	// Upload the gathered matrices to the instanceVBO
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(mat4), instanceMatrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	bind(shader);

	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr, instanceCount);
	glBindVertexArray(0);
}

void Mesh::bind(const Shader& shader) const
{
	// Bind Diffuse Handles SSBO to binding point 2
	if(diffuseHandlesSSBO != 0)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, diffuseHandlesSSBO);

	// Bind Specular Handles SSBO to binding point 3
	if(specularHandlesSSBO != 0)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, specularHandlesSSBO);

	// Bind Normal Handles SSBO to binding point 4
	if(normalHandlesSSBO != 0)
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, normalHandlesSSBO);

	// Set counts
	shader.setInt("u_numDiffuseTextures", static_cast<int>(diffuseHandles.size()));
	shader.setInt("u_numSpecularTextures", static_cast<int>(specularHandles.size()));
	shader.setInt("u_numNormalTextures", static_cast<int>(normalHandles.size()));
}

void Mesh::cleanup()
{
	// NOTE: Textures are NOT deleted here because they are shared across meshes
	// and owned by the Model's registry. Model::~Model() handles texture cleanup.

	// Delete SSBOs (these are per-mesh, so we delete them here)
	if(diffuseHandlesSSBO != 0)
	{
		glDeleteBuffers(1, &diffuseHandlesSSBO);
		diffuseHandlesSSBO = 0;
	}
	if(specularHandlesSSBO != 0)
	{
		glDeleteBuffers(1, &specularHandlesSSBO);
		specularHandlesSSBO = 0;
	}
	if(normalHandlesSSBO != 0)
	{
		glDeleteBuffers(1, &normalHandlesSSBO);
		normalHandlesSSBO = 0;
	}


	// Delete VAO, VBO, EBO
	if(VAO != 0)
	{
		glDeleteVertexArrays(1, &VAO);
		VAO = 0;
	}
	if(VBO != 0)
	{
		glDeleteBuffers(1, &VBO);
		VBO = 0;
	}
	if(EBO != 0)
	{
		glDeleteBuffers(1, &EBO);
		EBO = 0;
	}
	if(instanceVBO != 0)
	{
		glDeleteBuffers(1, &instanceVBO);
		instanceVBO = 0;
	}

	diffuseHandles.clear();
	specularHandles.clear();
	normalHandles.clear();
	textures.clear();
}

Mesh::~Mesh()
{
	cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept
: vertices(std::move(other.vertices)),
  indices(std::move(other.indices)),
  textures(std::move(other.textures)),
  VAO(other.VAO),
  VBO(other.VBO),
  EBO(other.EBO),
  instanceVBO(other.instanceVBO),
  diffuseHandlesSSBO(other.diffuseHandlesSSBO),
  specularHandlesSSBO(other.specularHandlesSSBO),
  normalHandlesSSBO(other.normalHandlesSSBO),
  diffuseHandles(std::move(other.diffuseHandles)),
  specularHandles(std::move(other.specularHandles)),
  normalHandles(std::move(other.normalHandles))
{
	// Nullify the source so it doesn't delete our resources
	other.VAO = 0;
	other.VBO = 0;
	other.EBO = 0;
	other.instanceVBO = 0;
	other.diffuseHandlesSSBO = 0;
	other.specularHandlesSSBO = 0;
	other.normalHandlesSSBO = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
	if(this != &other)
	{
		// Clean up our current resources
		cleanup();

		// Move data from other
		vertices = std::move(other.vertices);
		indices = std::move(other.indices);
		textures = std::move(other.textures);
		VAO = other.VAO;
		VBO = other.VBO;
		EBO = other.EBO;
		instanceVBO = other.instanceVBO;
		diffuseHandlesSSBO = other.diffuseHandlesSSBO;
		specularHandlesSSBO = other.specularHandlesSSBO;
		normalHandlesSSBO = other.normalHandlesSSBO;
		diffuseHandles = std::move(other.diffuseHandles);
		specularHandles = std::move(other.specularHandles);
		normalHandles = std::move(other.normalHandles);

		// Nullify the source
		other.VAO = 0;
		other.VBO = 0;
		other.EBO = 0;
		other.instanceVBO = 0;
		other.diffuseHandlesSSBO = 0;
		other.specularHandlesSSBO = 0;
		other.normalHandlesSSBO = 0;
	}
	return *this;
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

GLuint TextureFromFile(const string& fullPath, GLuint64& outHandle)
{
	unsigned int textureID = 0;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
	if(data)
	{
		if(!ProcessTexture(data, width, height, nrComponents, textureID))
			cout << "Failed to process texture at path: " << fullPath << endl;
		stbi_image_free(data);
	}
	else
	{
		cout << "TextureComponent failed to load at path: " << fullPath << endl;
		stbi_image_free(data);
	}

	// Get bindless texture handle and make it resident
	outHandle = glGetTextureHandleARB(textureID);
	glMakeTextureHandleResidentARB(outHandle);

	return textureID;
}

Model::Model(const string& modelPath)
{
	cout << "------------------Model-------------------" << endl;
	loadModel(modelPath);
	cout << "Number of meshes: " << registry.view<Mesh>().storage()->size() << endl;
	const auto texturesView = registry.view<TextureComponent>();
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
		| aiProcess_CalcTangentSpace
		| aiProcess_JoinIdenticalVertices
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
	vector<Index> indices; // Declare indices in the correct scope
	vector<TextureComponent> textures; // Declare textures in the correct scope

	for(unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex{};
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

		// tangent
		if(mesh->HasTangentsAndBitangents())
		{
			aiVector3D transformedTangent = transform * mesh->mTangents[i];
			vertex.Tangent = vec3(transformedTangent.x, transformedTangent.y, transformedTangent.z);
		}

		vertices.push_back(vertex);
	}

	for(unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for(unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

	// Load diffuse/base color textures (try both for compatibility)
	vector<TextureComponent> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse", scene);
	vector<TextureComponent> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specular", scene);
	vector<TextureComponent> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normal", scene);
	if(normalMaps.empty())
		normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "normal", scene);

	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

	Mesh& meshComp = registry.emplace<Mesh>(registry.create());
	meshComp.setup(vertices, indices, textures);
}

vector<TextureComponent> Model::loadMaterialTextures(const aiMaterial* mat, aiTextureType type, const string& typeName,
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
						const unsigned int size = atex->mWidth;
						data = stbi_load_from_memory(
							static_cast<unsigned char*>(const_cast<void*>(reinterpret_cast<const void*>(atex->
								pcData))), static_cast<int>(size), &width, &height, &nrComponents, 0);
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
					texture.handle = 0;
					if(data)
					{
						if(!ProcessTexture(data, width, height, nrComponents, textureID))
							cout << "Failed to process embedded texture at path: " << texPath << endl;
						dataAllocated ? free(data) : stbi_image_free(data);
						texture.id = textureID;

						// Get bindless texture handle and make it resident
						texture.handle = glGetTextureHandleARB(textureID);
						glMakeTextureHandleResidentARB(texture.handle);
					}

					texture.type = typeName;
					texture.path = texPath;
					textures.push_back(texture);
					registry.emplace<TextureComponent>(registry.create(), texture);
					continue; // processed embedded texture, go to next
				}
				cout << "Embedded texture index out of range: " << texPath << endl;
			}

			// Check if texture is embedded in the scene (FBX files often embed textures with paths like "Cardboard_Box.fbm/texture.jpg")
			const aiTexture* embeddedTex = nullptr;
			if(scene->mTextures && scene->mNumTextures > 0)
			{
				// Search for embedded texture by filename
				for(unsigned int t = 0; t < scene->mNumTextures; ++t)
				{
					const aiTexture* atex = scene->mTextures[t];
					if(atex->mFilename.length > 0)
					{
						// Compare the texture path with the embedded texture filename
						string embeddedName = string(atex->mFilename.C_Str());
						string texPathStr = string(str.C_Str());

						// Check if the paths match (exact match or basename match)
						if(embeddedName == texPathStr ||
							embeddedName.find(texPathStr) != string::npos ||
							texPathStr.find(embeddedName) != string::npos)
						{
							embeddedTex = atex;
							break;
						}
					}
				}
			}

			if(embeddedTex)
			{
				// Load embedded texture
				unsigned int textureID;
				glGenTextures(1, &textureID);
				glBindTexture(GL_TEXTURE_2D, textureID);

				int width = 0, height = 0, nrComponents = 0;
				unsigned char* data = nullptr;
				bool dataAllocated = false;

				if(embeddedTex->mHeight == 0)
				{
					// compressed image format (PNG/JPEG) inside memory
					const unsigned int size = embeddedTex->mWidth;
					data = stbi_load_from_memory(
						static_cast<unsigned char*>(const_cast<void*>(reinterpret_cast<const void*>(embeddedTex->
							pcData))),
						static_cast<int>(size), &width, &height, &nrComponents, 0);
				}
				else
				{
					// uncompressed RGBA32 image stored as aiTexel array
					width = static_cast<int>(embeddedTex->mWidth);
					height = static_cast<int>(embeddedTex->mHeight);
					nrComponents = 4;
					const size_t bufSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
					data = static_cast<unsigned char*>(malloc(bufSize));
					dataAllocated = true;
					const auto texels = reinterpret_cast<aiTexel*>(embeddedTex->pcData);
					for(size_t p = 0; p < static_cast<size_t>(width) * static_cast<size_t>(height); ++p)
					{
						data[p * 4 + 0] = texels[p].r;
						data[p * 4 + 1] = texels[p].g;
						data[p * 4 + 2] = texels[p].b;
						data[p * 4 + 3] = texels[p].a;
					}
				}

				texture.id = 0;
				texture.handle = 0;
				if(data)
				{
					if(!ProcessTexture(data, width, height, nrComponents, textureID))
						cout << "Failed to process embedded texture at path: " << str.C_Str() << endl;
					dataAllocated ? free(data) : stbi_image_free(data);
					texture.id = textureID;

					// Get bindless texture handle and make it resident
					texture.handle = glGetTextureHandleARB(textureID);
					glMakeTextureHandleResidentARB(texture.handle);
				}

				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				registry.emplace<TextureComponent>(registry.create(), texture);
			}
			else
			{
				// fallback: try loading texture from file on disk
				texture.id = TextureFromFile(this->directory + "/" + str.C_Str(), texture.handle);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				registry.emplace<TextureComponent>(registry.create(), texture);
				// store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
			}
		}
	}
	return textures;
}

Model::~Model()
{
	// Check if this Model was moved-from (registry is empty/invalid after move)
	// We check by seeing if there's any storage at all
	if(registry.storage<entt::entity>().empty())
		return;

	// First, clean up all textures (they are shared across meshes)
	auto texturesView = registry.view<TextureComponent>();
	for(auto [ent, tex] : texturesView.each())
	{
		// Make bindless handle non-resident before deleting
		if(tex.handle != 0)
			glMakeTextureHandleNonResidentARB(tex.handle);

		// Delete the OpenGL texture
		if(tex.id != 0)
			glDeleteTextures(1, &tex.id);
	}

	// Clear the registry - this will destroy Mesh components which clean up their own VAO/VBO/EBO/SSBOs
	registry.clear();
}

Model::Model(Model&& other) noexcept
: directory(std::move(other.directory)),
  registry(std::move(other.registry))
{
	// Mark the source as moved-from by clearing its directory
	other.directory.clear();
}

Model& Model::operator=(Model&& other) noexcept
{
	if(this != &other)
	{
		// Clean up our current resources first (same logic as destructor)
		if(!registry.storage<entt::entity>().empty())
		{
			auto texturesView = registry.view<TextureComponent>();
			for(auto [ent, tex] : texturesView.each())
			{
				if(tex.handle != 0)
					glMakeTextureHandleNonResidentARB(tex.handle);
				if(tex.id != 0)
					glDeleteTextures(1, &tex.id);
			}
			registry.clear();
		}

		// Move from other
		directory = std::move(other.directory);
		registry = std::move(other.registry);

		// Mark the source as moved-from
		other.directory.clear();
	}
	return *this;
}

void Model::drawInstanced(const Shader& shader, const vector<mat4>& instanceMatrices) const
{
	const auto view = registry.view<Mesh>();
	view.each([&shader, &instanceMatrices](const Mesh& mesh)
	{
		mesh.drawInstanced(shader, instanceMatrices);
	});
}
