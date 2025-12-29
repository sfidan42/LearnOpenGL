#pragma once
#include <string>
#include <glad/glad.h>
#include "Shader.hpp"

using namespace std;

GLuint CubemapFromFile(const string textureFacePaths[6]);

class Skybox
{
public:
	Skybox();
	~Skybox();

	void loadFaces(const string& directory, const string facePaths[6]);
	void draw(const Shader& shader) const;
private:
	GLuint skyboxTextureID = 0;
	vector<float> skyboxVertices;
	GLuint skyboxVAO = 0;
	GLuint skyboxVBO = 0;
};
