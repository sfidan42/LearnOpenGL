#include "Model.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stb_image.h>

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
		cout << "Texture failed to load at path: " << path << endl;
		stbi_image_free(data);
	}

	return textureID;
}

Model::Model(const string& modelPath)
{
	cout << "------------------Model-------------------" << endl;
	loadModel(modelPath);
	cout << "Number of meshes: " << registry.view<MeshComponent>().storage()->size() << endl;
	cout << "Number of textures loaded: " << textures_loaded.size() << endl;
	for(auto& [id, type, path] : textures_loaded)
		cout << "\tTexture ID: " << id << ", Type: " << type << ", Path: " << path << endl;
	cout << "----------------------------------------" << endl;
}

Model::~Model()
{
	// TODO: properly free model data
}

void Model::draw(const Shader& shader) const
{
	const auto view = registry.view<MeshComponent>();
	view.each([shader](const MeshComponent& mesh){ mesh.draw(shader); });
}

void Model::loadModel(const string& modelPath)
{
	// read file via ASSIMP
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(
		modelPath,
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_TransformUVCoords | aiProcess_FlipUVs);
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

void Model::processNode(aiNode* node, const aiScene* scene)
{
	// process each mesh located at the current node
	for(unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		// the node object only contains indices to index the actual objects in the scene.
		// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene);
	}
	// after we've processed all the meshes (if any) we then recursively process each of the children nodes
	for(unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene);
	}
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
	// data to fill
	vector<Vertex> vertices;
	vector<unsigned int> indices;
	vector<Texture> textures;

	// walk through each of the mesh's vertices
	for(unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex vertex{}; // initialize to avoid uninitialized warnings
		vec3 vector;
		// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder vec3 first.
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
			vec2 vec;
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoords = vec;
		}
		else
			vertex.TexCoords = vec2(0.0f, 0.0f);

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
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	// 2. specular maps
	vector<Texture> specularMaps =
		loadMaterialTextures(material, aiTextureType_SPECULAR, "specular", scene);
	textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

	// return a mesh object created from the extracted mesh data
	entt::entity entity = registry.create();
	MeshComponent& meshComp = registry.emplace<MeshComponent>(entity);
	meshComp.setup(vertices, indices, textures);
}

vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const string& typeName,
											const aiScene* scene)
{
	vector<Texture> textures;
	for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for(auto& j : textures_loaded)
		{
			if(strcmp(j.path.data(), str.C_Str()) == 0)
			{
				textures.push_back(j);
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
					textures_loaded.push_back(texture);
					continue; // processed embedded texture, go to next
				}
				cout << "Embedded texture index out of range: " << texPath << endl;
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

Model::Model(Model&& other) noexcept
    : textures_loaded(std::move(other.textures_loaded)),
      directory(std::move(other.directory)),
      registry(std::move(other.registry)) {}

Model& Model::operator=(Model&& other) noexcept {
    if (this != &other) {
        textures_loaded = std::move(other.textures_loaded);
        directory = std::move(other.directory);
        registry = std::move(other.registry);
    }
    return *this;
}
