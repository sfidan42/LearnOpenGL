#pragma once
#include <glad/glad.h>
#include <assimp/scene.h>
#include <string>
#include <vector>
#include "Shader.hpp"
#include <iostream>
#include "Primitives.hpp"
#include <entt/entity/registry.hpp>

struct TextureComponent;

class Mesh
{
public:
	void setup(const vector<Vertex>& vertices, const vector<Index>& indices, const vector<TextureComponent>& textures);
	void drawInstanced(const Shader& shader, const vector<mat4>& matrices) const;

private:
	void bind(const Shader& shader) const;

	vector<Vertex> vertices;
	vector<Index> indices;
	vector<TextureComponent> textures;
	GLuint VAO{}, VBO{}, EBO{};
	GLuint instanceVBO{};
};

bool ProcessTexture(unsigned char* data, int width, int height, int nrComponents, GLuint& textureID);
GLuint TextureFromFile(const string& fullPath);

class Model
{
public:
	explicit Model(const string& modelPath);
	~Model() = default;

	// Delete copy constructor and copy assignment operator
	Model(const Model&) = delete;
	Model& operator=(const Model&) = delete;

	// Implement move constructor and move assignment operator
	Model(Model&& other) noexcept;
	Model& operator=(Model&& other) noexcept;

	void drawInstanced(const Shader& shader, const vector<mat4>& matrices) const;

private:
	void loadModel(const string& modelPath);
	void processNode(aiNode* node, const aiScene* scene, const aiMatrix4x4& parentTransform = aiMatrix4x4());
	void processMesh(aiMesh* mesh, const aiScene* scene, const aiMatrix4x4& transform);
	vector<TextureComponent> loadMaterialTextures(aiMaterial* mat, aiTextureType type,
												  const string& typeName, const aiScene* scene);
	string directory;
	entt::registry registry;
};
