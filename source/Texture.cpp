#include "Texture.hpp"

#include <iostream>
#include <stb_image.h>

bool Texture::load(const std::string& imagePath)
{
	std::string data_dir = DATA_DIR;
	std::string full_path = data_dir + "/" + imagePath;
	unsigned char *data = stbi_load(full_path.c_str(), &size.x, &size.y, &nrChannels, 0);
	if (data)
	{
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		GLint format;
		switch(nrChannels)
		{
			case 1: format = GL_RED; break;
			case 3: format = GL_RGB; break;
			case 4: format = GL_RGBA; break;
			default: format = GL_RGB; break;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data);
		return true;
	}
	else
	{
		std::cout << "Failed to load texture: " << full_path << std::endl;
		return false;
	}
}

void Texture::bind(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::unbind() const
{
	glBindTexture(GL_TEXTURE_2D, textureID);
}

Texture::~Texture()
{
	glDeleteTextures(1, &textureID);
}
