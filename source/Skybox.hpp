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
	void scale(float scale);
	void draw(const Shader& shader) const;
private:
	GLuint skyboxTextureID = 0;
	float scaleFactor = 1.0f;
	vector<float> skyboxVertices;
	GLuint skyboxVAO = 0;
	GLuint skyboxVBO = 0;
};
