#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#include "shaders.h"

std::string textFileRead(const std::string fn)
{
	std::ifstream file(fn);
	std::string content;
	std::stringstream ss;
	if (file.is_open())
	{
		while (file >> content) {
			ss << content << std::endl;
		}
	}
	else
	{
		std::cerr << "Error opening file: " << fn << std::endl;
		exit(EXIT_FAILURE);
	}
	return ss.str();
}

std::string getShaderInfoLog(const GLuint obj)
{
	int infologLength = 0;
	std::string s;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 0)
	{
		std::vector<char> v(infologLength);
		glGetShaderInfoLog(obj, infologLength, NULL, v.data());
		s.assign(begin(v), end(v));
	}
	return s;
}

std::string getProgramInfoLog(const GLuint obj)
{
	int infologLength = 0;
	std::string s;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 0)
	{
		std::vector<char> v(infologLength);
		glGetProgramInfoLog(obj, infologLength, NULL, v.data());
		s.assign(begin(v), end(v));
	}

	return s;
}

GLuint compile_shader(const std::string source_file, const GLenum type)
{
	std::string src = textFileRead(source_file);
	const char* str_ptr = src.c_str();

	GLuint id = glCreateShader(type);

	glShaderSource(id, 1, &str_ptr, NULL);
	glCompileShader(id);
	std::cout << getShaderInfoLog(id) << std::endl;

	return id;
}

GLuint link_shader(const std::vector<GLuint> shader_ids)
{
	GLuint id = glCreateProgram();
	
	for (auto i : shader_ids)
		glAttachShader(id, i);

	glLinkProgram(id);
	std::cout << getProgramInfoLog(id) << std::endl;

	return id;
}
