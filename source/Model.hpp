#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <string>
#include <vector>
#include "Mesh.hpp"
#include "Shader.hpp"
#include <iostream>

using namespace std;
using namespace glm;

bool ProcessTexture(unsigned char* data, int width, int height, int nrComponents, GLuint& textureID);
unsigned int TextureFromFile(const char* path, const string& directory);

class Model
{
public:
	explicit Model(const string& modelPath);
	~Model();

	void draw(const Shader& shader) const;

private:
	vector<Texture> textures_loaded;
	vector<Mesh> meshes;
	vector<Material*> materials;
	string directory;

	void loadModel(const string& modelPath);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
										 const string& typeName, const aiScene* scene);

	Material* addMaterial(Material* inMat);
};
