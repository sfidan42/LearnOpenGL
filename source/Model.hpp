#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <string>
#include <vector>
#include "Mesh.hpp"
#include "Shader.hpp"
#include "stb_image.h"
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>

using namespace std;

void processTexture(unsigned char* data, int width, int height, int nrComponents);
unsigned int TextureFromFile(const char* path, const string& directory);

class Model
{
public:
	// model data
	vector<Texture> textures_loaded;
	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
	vector<Mesh> meshes;
	vector<Material*> materials;
	string directory;

	Model(const string& path)
	{
		loadModel(path);
		std::cout << "Model loaded successfully from: " << path << std::endl;
		std::cout << "Number of meshes: " << meshes.size() << std::endl;
		std::cout << "Number of textures loaded: " << textures_loaded.size() << std::endl;
		std::cout << "----------------------------------------" << std::endl;
		for(Texture& tex : textures_loaded)
		{
			std::cout << "Texture ID: " << tex.id << ", Type: " << tex.type << ", Path: " << tex.path << std::endl;
		}
		std::cout << "----------------------------------------" << std::endl;
	}

	void draw(const Shader& shader)
	{
		for(auto& mesh : meshes)
			mesh.draw(shader);
	}

private:
	void loadModel(const string& path)
	{
		// read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_TransformUVCoords | aiProcess_FlipUVs);
		// check for errors
		if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// retrieve the directory path of the filepath
		directory = path.substr(0, path.find_last_of('/'));

		// process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		// process each mesh located at the current node
		for(unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the node object only contains indices to index the actual objects in the scene.
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(processMesh(mesh, scene));
		}
		// after we've processed all the meshes (if any) we then recursively process each of the children nodes
		for(unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		// data to fill
		vector<Vertex> vertices;
		vector<unsigned int> indices;
		Material* mat = new Material();

		// walk through each of the mesh's vertices
		for(unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex{}; // initialize to avoid uninitialized warnings
			glm::vec3 vector;
			// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if(mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for(unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for(unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		// process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN
		// normal: texture_normalN

		// 1. diffuse maps
		vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuse", scene);
		mat->textures.insert(mat->textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		// 2. specular maps
		vector<Texture> specularMaps =
			loadMaterialTextures(material, aiTextureType_SPECULAR, "specular", scene);
		mat->textures.insert(mat->textures.end(), specularMaps.begin(), specularMaps.end());

		// return a mesh object created from the extracted mesh data
		return {vertices, indices, addMaterial(materials, mat)};
	}

	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
										 const string& typeName, const aiScene* scene)
	{
		vector<Texture> textures;
		for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			bool skip = false;
			for(unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;
					// a texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}
			if(!skip)
			{
				// if texture hasn't been loaded already, load it
				Texture texture;

				const char* texPath = str.C_Str();
				// Handle embedded textures in glTF/GLB: paths like "*0"
				if(texPath && texPath[0] == '*')
				{
					// parse index after '*'
					char* endptr = nullptr;
					long texIndexLong = strtol(texPath + 1, &endptr, 10);
					int texIndex = (int)texIndexLong;
					if(endptr != (texPath + 1) && scene->mTextures && texIndex >= 0 && texIndex < (int)scene->
						mNumTextures)
					{
						aiTexture* atex = scene->mTextures[texIndex];

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
								reinterpret_cast<unsigned char*>(const_cast<void*>(reinterpret_cast<const void*>(atex->
									pcData))), (int)size, &width, &height, &nrComponents, 0);
						}
						else
						{
							// uncompressed RGBA32 image stored as aiTexel array
							width = static_cast<int>(atex->mWidth);
							height = static_cast<int>(atex->mHeight);
							nrComponents = 4; // aiTexel has r,g,b,a
							size_t bufSize = (size_t)width * (size_t)height * 4;
							data = (unsigned char*)malloc(bufSize);
							dataAllocated = true;
							auto texels = reinterpret_cast<aiTexel*>(atex->pcData);
							for(size_t p = 0; p < (size_t)width * (size_t)height; ++p)
							{
								data[p * 4 + 0] = texels[p].r;
								data[p * 4 + 1] = texels[p].g;
								data[p * 4 + 2] = texels[p].b;
								data[p * 4 + 3] = texels[p].a;
							}
						}

						if(data)
						{
							// Choose proper internal format and data format based on number of components
							GLint format;
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

							if(format == -1)
							{
								std::cout << "Unsupported number of image components: " << nrComponents << std::endl;
							}
							else
							{
								glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
											 GL_UNSIGNED_BYTE, data);
							}
							glGenerateMipmap(GL_TEXTURE_2D);

							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
							glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

							if(nrComponents == 0)
								std::cout << "Warning: embedded texture had 0 components" << std::endl;

							// free image data
							if(dataAllocated)
								free(data);
							else
								stbi_image_free(data);

							texture.id = textureID;
						}
						else
						{
							std::cout << "Embedded texture failed to load at path: " << texPath << std::endl;
							texture.id = 0;
						}

						texture.type = typeName;
						texture.path = texPath;
						textures.push_back(texture);
						textures_loaded.push_back(texture);
						continue; // processed embedded texture, go to next
					}
					std::cout << "Embedded texture index out of range: " << texPath << std::endl;
				}

				// fallback: try loading texture from file on disk
				texture.id = TextureFromFile(str.C_Str(), this->directory);
				texture.type = typeName;
				texture.path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture);
				// store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
			}
		}
		return textures;
	}
};

inline void processTexture(unsigned char* data, int width, int height, int nrComponents)
{

}

inline unsigned int TextureFromFile(const char* path, const string& directory)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID = 0;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if(data)
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
			std::cout << "Unsupported number of image components when loading '" << path << "': " << nrComponents <<
				std::endl;
		else
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}
