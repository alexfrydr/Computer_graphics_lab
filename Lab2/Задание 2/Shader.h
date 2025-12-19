#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <string>

class Shader {
public:
    GLuint Program;
    Shader(const char* vertexPath, const char* fragmentPath);
    void Use();
    void setFloat(const std::string &name, float value);
    void setVec3(const std::string &name, float v1, float v2, float v3);
    void setVec4(const std::string &name, float v1, float v2, float v3, float v4);
};

#endif