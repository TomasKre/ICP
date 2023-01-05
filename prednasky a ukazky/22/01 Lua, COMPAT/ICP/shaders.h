#pragma once

#include <string>
#include <GL/glew.h> 

std::string textFileRead(const std::string fn);
std::string getShaderInfoLog(const GLuint obj);
std::string getProgramInfoLog(const GLuint obj);

GLuint compile_shader(const std::string source_file, const GLenum type);
GLuint link_shader(const std::vector<GLuint> shader_ids);
