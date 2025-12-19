#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const char* path) {
    std::string code;
    std::ifstream file(path, std::ios::in);
    if (file.is_open()) {
        std::stringstream stream;
        stream << file.rdbuf();
        code = stream.str();
        file.close();
    }
    return code;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    Program = glCreateProgram();
    glAttachShader(Program, vertex);
    glAttachShader(Program, fragment);
    glLinkProgram(Program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Use() {
    glUseProgram(Program);
}

void Shader::setFloat(const std::string &name, float value) {
    glUniform1f(glGetUniformLocation(Program, name.c_str()), value);
}

void Shader::setVec3(const std::string &name, float v1, float v2, float v3) {
    glUniform3f(glGetUniformLocation(Program, name.c_str()), v1, v2, v3);
}

void Shader::setVec4(const std::string &name, float v1, float v2, float v3, float v4) {
    glUniform4f(glGetUniformLocation(Program, name.c_str()), v1, v2, v3, v4);
}