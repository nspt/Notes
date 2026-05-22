#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <string>

class ShaderProgram {
public:
    ShaderProgram(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    ~ShaderProgram();
    void use() const;
    void setUniform(const std::string &name, int value) const;
    void setUniform(const std::string &name, float value) const;
private:
    unsigned int m_id;
};

#endif