#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>

class Shader
{
public:
	Shader() = default;
	~Shader() = default;

	void load_shaders(const std::vector<std::string>& filepaths);
	void use(unsigned int index);

	void setMat4fv(const std::string& name, const float* value) const;
	void set3f(const std::string& name, float a, float b, float c) const;

private:
	void config(GLuint count);
	void read(const std::string& filepath);
	void create();

	struct ShaderSource
	{
		std::string vertex;
		std::string fragment;
	};

	GLuint program = -1;
	std::vector<GLuint> programs;
	std::vector<ShaderSource> shaders;
};
