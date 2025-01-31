#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

GLuint createShaderModule(std::string path, GLenum type)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::stringstream buffer {};
    buffer << file.rdbuf();

    std::string string = buffer.str();
    auto data = string.data();

    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    GLint isSuccess;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isSuccess);

    if (!isSuccess)
    {
        GLint messageLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &messageLength);

        std::string message {};
        message.resize(messageLength);

        glGetShaderInfoLog(shader, message.size(), nullptr, message.data());

        throw std::runtime_error("Failed to compile shader (" + path + "): " + message);
    }

    return shader;
}

GLuint createGraphicsProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
{
    GLuint vertexModule = createShaderModule(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentModule = createShaderModule(fragmentShaderPath, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexModule);
    glAttachShader(program, fragmentModule);
    {
        glLinkProgram(program);

        GLint isSuccess;
        glGetProgramiv(program, GL_LINK_STATUS, &isSuccess);

        if (!isSuccess)
        {
            GLint messageLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &messageLength);

            std::string message {};
            message.resize(messageLength);

            glGetProgramInfoLog(program, message.size(), nullptr, message.data());

            throw std::runtime_error("Failed to link shader program: " + message);
        }
    }
    glDetachShader(program, vertexModule);
    glDetachShader(program, fragmentModule);
    glDeleteShader(vertexModule);
    glDeleteShader(fragmentModule);

    return program;
}

// Loads, compiles, and links a compute shader
GLuint createComputeProgram(std::string path)
{
    GLuint module = createShaderModule(path, GL_COMPUTE_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, module);
    {
        glLinkProgram(program);

        GLint isSuccess;
        glGetProgramiv(program, GL_LINK_STATUS, &isSuccess);

        if (!isSuccess)
        {
            GLint messageLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &messageLength);

            std::string message {};
            message.resize(messageLength);

            glGetProgramInfoLog(program, message.size(), nullptr, message.data());

            throw std::runtime_error("Failed to link shader program: " + message);
        }
    }
    glDetachShader(program, module);
    glDeleteShader(module);

    return program;
}
