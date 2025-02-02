#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//Loads and compiles a shader file
GLuint createShaderModule(std::string path, GLenum type)
{
    //Load the shader file
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + path);
    }

    //Read the shader file contents
    std::stringstream buffer {};
    buffer << file.rdbuf();

    std::string string = buffer.str();
    auto data = string.data();
    //We now have the shader file contents

    //Create and compile a new shader using the shader file contentsw
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    //Verify that compilation was successful
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
    
    //Return the GLuint that refers to the shader
    return shader;
}

//Assembles a vertex and fragment shader into a graphics program that can be used for rendering
GLuint createGraphicsProgram(std::string vertexShaderPath, std::string fragmentShaderPath)
{
    //Load the shaders
    //TODO: consider having a shader cache for already compiled shaders, so that recompilation does not need to be done every time a new program reuses the same shader. (performance optimization at the cost of memory usage)
    GLuint vertexModule = createShaderModule(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentModule = createShaderModule(fragmentShaderPath, GL_FRAGMENT_SHADER);

    //Create the program object and bind the two shaders to it.
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexModule);
    glAttachShader(program, fragmentModule);
    {
        //Link the program together (this is akin to compiling an individual shader)
        glLinkProgram(program);

        //Verify that program linking was successful
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
    //Unload the shaders and delete them to free up memory.
    glDetachShader(program, vertexModule);
    glDetachShader(program, fragmentModule);
    glDeleteShader(vertexModule);
    glDeleteShader(fragmentModule);

    return program;
}

// Loads, compiles, and links a compute shader
GLuint createComputeProgram(std::string path)
{
    //Load the compute shader
    GLuint module = createShaderModule(path, GL_COMPUTE_SHADER);

    //Create the compute program
    GLuint program = glCreateProgram();
    glAttachShader(program, module);
    {
        //Link the compute program
        glLinkProgram(program);

        //Verify that linking was successfull
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
    
    //Unload the shader and delete it to free up memory.
    glDetachShader(program, module);
    glDeleteShader(module);

    return program;
}
