#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader
{
public:
	Shader() = default;
	~Shader();

	bool loadShaders(const std::vector<std::string>& filepaths);
	void use(unsigned int index);

	void setMat4(const std::string& name, const glm::mat4& matrix) const;
	void setVec3(const std::string& name, const glm::vec3& vec) const;
	void setFloat(const std::string& name, float value) const;
	void setInt(const std::string& name, int value) const;
	void setBool(const std::string& name, int value) const;

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
