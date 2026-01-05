#include "Skybox.hpp"
#include <stb_image.h>
#include <iostream>
#include "error_macro.hpp"

GLuint CubemapFromFile(const string& directory, const string textureFacePaths[6])
{
	GLuint textureID;
	GL_CHECK(glGenTextures(1, &textureID));
	GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, textureID));

	int width, height, nrChannels;
	for (unsigned int i = 0; i < 6; i++)
	{
		const string path = directory + "/" + textureFacePaths[i];
		unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			GLint format = GL_RGB;
			if (nrChannels == 1)
				format = GL_RED;
			else if (nrChannels == 3)
				format = GL_RGB;
			else if (nrChannels == 4)
				format = GL_RGBA;

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

Skybox::Skybox()
{
	skyboxVertices = {
		// positions
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	GL_CHECK(glGenVertexArrays(1, &skyboxVAO));
	GL_CHECK(glGenBuffers(1, &skyboxVBO));
	GL_CHECK(glBindVertexArray(skyboxVAO));
	GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO));
	GL_CHECK(glBufferData(GL_ARRAY_BUFFER, skyboxVertices.size() * sizeof(float), &skyboxVertices[0], GL_STATIC_DRAW));
	GL_CHECK(glEnableVertexAttribArray(0));
	GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(nullptr)));
	GL_CHECK(glBindVertexArray(0));
}

Skybox::~Skybox()
{
	glDeleteTextures(1, &skyboxTextureID);
	if (skyboxVAO)
		glDeleteVertexArrays(1, &skyboxVAO);
	if (skyboxVBO)
		glDeleteBuffers(1, &skyboxVBO);
}

void Skybox::loadFaces(const string& directory, const string facePaths[6])
{
	if (skyboxTextureID)
		glDeleteTextures(1, &skyboxTextureID);
	skyboxTextureID = CubemapFromFile(directory, facePaths);
}

void Skybox::scale(const float scale)
{
	scaleFactor = scale;
}

void Skybox::draw(const Shader& shader) const
{
	glDepthFunc(GL_LEQUAL);
	shader.use();
	shader.setInt("cubemap", 0);
	shader.setFloat("scaleFactor", scaleFactor);
	GL_CHECK(glBindVertexArray(skyboxVAO));
	GL_CHECK(glActiveTexture(GL_TEXTURE0));
	GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID));
	GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, skyboxVertices.size()));
	GL_CHECK(glBindVertexArray(0));
	GL_CHECK(glDepthFunc(GL_LESS));
}
