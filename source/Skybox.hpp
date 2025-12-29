#pragma once
#include <string>
#include <glad/glad.h>

using namespace std;

unsigned int CubemapFromFile(const string& path);

class Skybox
{
public:
	Skybox() = default;
	~Skybox();

	void loadTextures(const string paths[6]);
private:
	GLuint skyboxTextureID = 0;
};
