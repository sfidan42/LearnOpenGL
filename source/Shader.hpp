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

	bool load(const string& filepath);
	void use() const;

	void setMat4(const string& name, const mat4& matrix) const;
	void setVec3(const string& name, const vec3& vec) const;
	void setFloat(const string& name, float value) const;
	void setInt(const string& name, int value) const;
	void setBool(const string& name, int value) const;

private:
	struct ShaderSource
	{
		string vertex;
		string fragment;
	};

	ShaderSource read(const string& filepath);
	static GLuint create(const ShaderSource& shaderCode);

	GLuint program = -1;
	ShaderSource source;
};
