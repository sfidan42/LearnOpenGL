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
	~Shader();

	bool load_shaders(const std::vector<std::string>& filepaths);
	void use(unsigned int index);

	[[nodiscard]] uint32_t get_num_shaders() const { return static_cast<uint32_t>(programs.size()); }

	void setMat4fv(const std::string& name, const float* value) const;
	void set3f(const std::string& name, float a, float b, float c) const;
	void set1i(const std::string& name, int value) const;

private:
	void config(GLuint count);
	void read(const std::string& filepath);
	bool create();

	struct ShaderSource
	{
		std::string vertex;
		std::string fragment;
	};

	GLuint program = -1;
	std::vector<GLuint> programs;
	std::vector<ShaderSource> shaders;
};
