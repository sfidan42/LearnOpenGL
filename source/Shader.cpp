#include "Shader.hpp"
#include "error_macro.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

bool Shader::loadShaders(const std::vector<std::string>& filepaths)
{
	config(filepaths.size());
	for(const std::string& filepath : filepaths)
		read(filepath);
	return create();
}

void Shader::config(const unsigned int count)
{
	programs.reserve(count);
	shaders.reserve(count);
}

void Shader::read(const std::string& filepath)
{
	std::string data_dir = DATA_DIR;
	std::string full_path = data_dir + "/" + filepath;

	std::string line;
	std::ifstream file(full_path);
	std::stringstream ss[2];

	if(!file.is_open())
	{
		std::cerr << "Failed to open file: " << full_path << "\n";
		return;
	}
	if(shaders.size() >= shaders.capacity())
	{
		std::cerr << "Shader count exceeded\n";
		return;
	}
	int i = -1;
	while(getline(file, line))
	{
		if(line.find("#shader") != std::string::npos)
		{
			if(line.find("vertex") != std::string::npos)
				i = 0;
			else if(line.find("fragment") != std::string::npos)
				i = 1;
		}
		else if(i != -1)
			ss[i] << line << '\n';
	}
	shaders.push_back({ss[0].str(), ss[1].str()});
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
		std::cerr << "shader compilation failed\n" << infoLog << std::endl;
		std::cerr << shader_source << std::endl;
		return false;
	}
	return true;
}

Shader::~Shader()
{
	for(const GLuint prog : programs)
		glDeleteProgram(prog);
}

void Shader::use(const unsigned int index)
{
	if(index >= programs.size())
	{
		std::cerr << "Invalid program index: " << index << std::endl;
		return;
	}
	program = programs[index];
	GL_CHECK(glUseProgram(program));
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniformMatrix4fv(loc, 1, GL_FALSE, &matrix[0][0]));
}

void Shader::setVec3(const std::string& name, const glm::vec3& vec) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform3fv(loc, 1, &vec[0]));
}

void Shader::setFloat(const std::string& name, const float value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform1f(loc, value));
}

void Shader::setInt(const std::string& name, const int value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform1i(loc, value));
}

void Shader::setBool(const std::string& name, const int value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	GL_CHECK(glUniform1i(loc, value));
}

bool Shader::create()
{
	int success;
	char infoLog[512];

	for(auto& [vertex, fragment] : shaders)
	{
		const char* vertexShaderSource = vertex.c_str();
		const char* fragmentShaderSource = fragment.c_str();
		const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		if(vertexShader == 0)
		{
			std::cerr << "Failed to create vertex shader" << std::endl;
			return false;
		}
		const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		if(fragmentShader == 0)
		{
			std::cerr << "Failed to create fragment shader" << std::endl;
			return false;
		}
		GLuint shaderProgram = glCreateProgram();
		if(shaderProgram == 0)
		{
			std::cerr << "Failed to create shader program" << std::endl;
			return false;
		}

		if(!compile(vertexShader, vertexShaderSource) || !compile(fragmentShader, fragmentShaderSource))
			return false;
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if(!success)
		{
			glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
			std::cerr << "Shader program linking failed\n" << infoLog << std::endl;
			return false;
		}
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
		programs.push_back(shaderProgram);
	}
	return true;
}
