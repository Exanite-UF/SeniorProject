#pragma once

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>
#include <string>

class ShaderProgram : public NonCopyable
{
public:
    GLuint programId;

    explicit ShaderProgram(GLuint programId);
    ~ShaderProgram() override;

    // Bind the shader program
    void use();

    // Utility functions for setting uniforms
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
};
