#include "Shader.hpp"
#include "error_macro.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

bool Shader::load(const string& filepath)
{
	source = read(filepath);
	if(source.vertex.empty() || source.fragment.empty())
	{
		cerr << "Failed to read shader from file: " << filepath << "\n";
		return false;
	}
	program = create(source);
	if(program == 0)
	{
		cerr << "Failed to create shader program from file: " << filepath << "\n";
		return false;
	}
	return true;
}

Shader::ShaderSource Shader::read(const string& filepath)
{
	string data_dir = DATA_DIR;
	string full_path = data_dir + "/" + filepath;

	string line;
	ifstream file(full_path);
	stringstream ss[3]; // 0=vertex, 1=geometry, 2=fragment

	if(!file.is_open())
	{
		cerr << "Failed to open file: " << full_path << "\n";
		return {};
	}
	int i = -1;
	while(getline(file, line))
	{
		if(line.find("#shader") != string::npos)
		{
			if(line.find("vertex") != string::npos)
				i = 0;
			else if(line.find("geometry") != string::npos)
				i = 1;
			else if(line.find("fragment") != string::npos)
				i = 2;
		}
		else if(i != -1)
			ss[i] << line << '\n';
	}
	return {ss[0].str(), ss[1].str(), ss[2].str()};
}

static bool compile(GLuint shader, const char* shader_source)
{
	int success;
	char infoLog[512];

	glShaderSource(shader, 1, &shader_source, nullptr);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		cerr << "shader compilation failed\n" << infoLog << endl;
		cerr << "Shader source:\n" << shader_source << endl;
		return false;
	}
	return true;
}

Shader::~Shader()
{
	glDeleteProgram(program);
}

void Shader::use() const
{
	GL_CHECK(glUseProgram(program));
}

void Shader::setMat4(const string& name, const mat4& matrix) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniformMatrix4fv(loc, 1, GL_FALSE, &matrix[0][0]));
}

void Shader::setVec3(const string& name, const vec3& vec) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform3fv(loc, 1, &vec[0]));
}

void Shader::setFloat(const string& name, const float value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform1f(loc, value));
}

void Shader::setInt(const string& name, const int value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform1i(loc, value));
}

void Shader::setBool(const string& name, const int value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform1i(loc, value));
}

GLuint Shader::create(const ShaderSource& shaderCode)
{
	int success;
	char infoLog[512];

	const char* vertexShaderSource = shaderCode.vertex.c_str();
	const char* fragmentShaderSource = shaderCode.fragment.c_str();
	const bool hasGeometry = !shaderCode.geometry.empty();

	const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	if(vertexShader == 0)
	{
		cerr << "Failed to create vertex shader" << endl;
		return 0;
	}
	const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	if(fragmentShader == 0)
	{
		cerr << "Failed to create fragment shader" << endl;
		return 0;
	}

	GLuint geometryShader = 0;
	if(hasGeometry)
	{
		geometryShader = glCreateShader(GL_GEOMETRY_SHADER);
		if(geometryShader == 0)
		{
			cerr << "Failed to create geometry shader" << endl;
			return 0;
		}
	}

	GLuint shaderProgram = glCreateProgram();
	if(shaderProgram == 0)
	{
		cerr << "Failed to create shader program" << endl;
		return 0;
	}

	if(!compile(vertexShader, vertexShaderSource))
	{
		cerr << "Vertex shader source (on failure):\n" << vertexShaderSource << endl;
		return 0;
	}
	if(hasGeometry && !compile(geometryShader, shaderCode.geometry.c_str()))
	{
		cerr << "Geometry shader source (on failure):\n" << shaderCode.geometry << endl;
		return 0;
	}
	if(!compile(fragmentShader, fragmentShaderSource))
	{
		cerr << "Fragment shader source (on failure):\n" << fragmentShaderSource << endl;
		return 0;
	}
	glAttachShader(shaderProgram, vertexShader);
	if(hasGeometry)
		glAttachShader(shaderProgram, geometryShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		cerr << "Shader program linking failed\n" << infoLog << endl;
		cerr << "Vertex shader source (on link fail):\n" << vertexShaderSource << endl;
		if(hasGeometry)
			cerr << "Geometry shader source (on link fail):\n" << shaderCode.geometry << endl;
		cerr << "Fragment shader source (on link fail):\n" << fragmentShaderSource << endl;
		return 0;
	}
	glDeleteShader(vertexShader);
	if(hasGeometry)
		glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}
