#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <string>
#include <vector>
#include "Ecs.hpp"
#include "Shader.hpp"
#include <iostream>
#include <entt/entity/registry.hpp>

using namespace std;
using namespace glm;

bool ProcessTexture(unsigned char* data, int width, int height, int nrComponents, GLuint& textureID);
unsigned int TextureFromFile(const char* path, const string& directory);

class Model
{
public:
	explicit Model(const string& modelPath);
	~Model();

	// Delete copy constructor and copy assignment operator
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	// Implement move constructor and move assignment operator
	Model(Model&& other) noexcept;
	Model& operator=(Model&& other) noexcept;

	void draw(const Shader& shader) const;

private:
	vector<Texture> textures_loaded;
	//vector<Mesh> meshes;
	string directory;

	entt::registry registry;

	void loadModel(const string& modelPath);
	void processNode(aiNode* node, const aiScene* scene);
	void processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
										 const string& typeName, const aiScene* scene);

};
