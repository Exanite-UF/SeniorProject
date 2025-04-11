#include "ShaderProgram.h"

ShaderProgram::ShaderProgram(const GLuint programId)
{
    this->programId = programId;
}

ShaderProgram::~ShaderProgram()
{
    glDeleteProgram(programId);
}

void ShaderProgram::use()
{
    glUseProgram(programId);
}

void ShaderProgram::setBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(programId, name.c_str()), (int)value);
}

void ShaderProgram::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(programId, name.c_str()), value);
}

void ShaderProgram::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(programId, name.c_str()), value);
}
