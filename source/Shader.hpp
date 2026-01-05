#pragma once
#include <vector>
#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

class Shader
{
public:
	Shader() = default;
	~Shader();

	// non-copyable, because of OpenGL resource management
	// Otherwise Shader destructor could be called during copy, which would delete the program ID
	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	bool load(const string& filepath);
	void use() const;

	void setMat4(const string& name, const mat4& matrix) const;
	void setVec3(const string& name, const vec3& vec) const;
	void setFloat(const string& name, float value) const;
	void setInt(const string& name, int value) const;
	void setBool(const string& name, int value) const;

	GLuint program = -1;

private:
	struct ShaderSource
	{
		string vertex;
		string geometry;
		string fragment;
	};

	ShaderSource read(const string& filepath);
	static GLuint create(const ShaderSource& shaderCode);

	ShaderSource source;
};
