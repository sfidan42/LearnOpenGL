#include "Shader.hpp"

bool Shader::load_shaders(const std::vector<std::string>& filepaths)
{
	config(filepaths.size());
	for (const std::string& filepath : filepaths)
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

bool Shader::create()
{
	int success;
	char infoLog[512];

	for(auto & shader : shaders)
	{
		const char* vertexShaderSource = shader.vertex.c_str();
		const char* fragmentShaderSource = shader.fragment.c_str();
		unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
		unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		unsigned int shaderProgram = glCreateProgram();

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

void Shader::use(unsigned int index)
{
	if(index >= programs.size())
	{
		std::cerr << "Invalid program index: " << index << std::endl;
		return;
	}
	program = programs[index];
	glUseProgram(program);
}

void Shader::setMat4fv(const std::string& name, const float* value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	glUniformMatrix4fv(loc, 1, GL_FALSE, value);
}

void Shader::set3f(const std::string& name, float a, float b, float c) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	glUniform3f(loc, a, b, c);
}

void Shader::set1f(const std::string& name, float value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	glUniform1f(loc, value);
}

void Shader::set1i(const std::string& name, int value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	glUniform1i(loc, value);
}

void Shader::set1b(const std::string& name, bool value) const
{
	const GLint loc = glGetUniformLocation(program, name.c_str());
	glUniform1i(loc, static_cast<int>(value));
}

Shader::~Shader()
{
	for (auto prog : programs) {
		glDeleteProgram(prog);
	}
}
