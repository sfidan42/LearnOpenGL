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

	bool loadShaders(const vector<string>& filepaths);
	void use(unsigned int index);

	void setMat4(const string& name, const mat4& matrix) const;
	void setVec3(const string& name, const vec3& vec) const;
	void setFloat(const string& name, float value) const;
	void setInt(const string& name, int value) const;
	void setBool(const string& name, int value) const;

private:
	void config(GLuint count);
	void read(const string& filepath);
	bool create();

	struct ShaderSource
	{
		string vertex;
		string fragment;
	};

	GLuint program = -1;
	vector<GLuint> programs;
	vector<ShaderSource> shaders;
};
