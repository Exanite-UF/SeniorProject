#include <fstream>
#include <src/graphics/ShaderManager.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>

ShaderManager* ShaderManager::instance = nullptr;

ShaderManager& ShaderManager::getInstance()
{
    if (instance == nullptr)
    {
        instance = new ShaderManager();
    }

    return *instance;
}

ShaderManager::ShaderManager() = default;

ShaderManager::~ShaderManager()
{
    for (const auto& computeProgram : computePrograms)
    {
        glDeleteProgram(computeProgram.second);
    }

    for (const auto& graphicsProgram : graphicsPrograms)
    {
        glDeleteProgram(graphicsProgram.second);
    }

    for (const auto& shaderModule : shaderModules)
    {
        glDeleteShader(shaderModule.second);
    }
}

GLuint ShaderManager::getShaderModule(const std::string_view& shaderPath, GLenum shaderType)
{
    // Use cached shader module if available
    auto cacheKey = std::make_tuple(std::string(shaderPath), shaderType);
    if (shaderModules.contains(cacheKey))
    {
        return shaderModules[cacheKey];
    }

    // Load the shader file
    std::ifstream file((std::string(shaderPath)));
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + std::string(shaderPath));
    }

    // Read the shader file contents
    std::stringstream buffer {};
    buffer << file.rdbuf();

    std::string string = buffer.str();
    auto data = string.data();
    // We now have the shader file contents

    // TODO: Add shader preprocessor (also modify the error message to mention that the shader has been preprocessed and that the error's line numbers cannot be trusted)

    // Create and compile a new shader using the shader file contents
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &data, nullptr);
    glCompileShader(shader);

    // Verify that compilation was successful
    GLint isSuccess;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isSuccess);

    if (!isSuccess)
    {
        GLint messageLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &messageLength);

        std::string message {};
        message.resize(messageLength);

        glGetShaderInfoLog(shader, message.size(), nullptr, message.data());

        std::cout << message.data() << std::endl;

        throw std::runtime_error("Failed to compile shader (" + std::string(shaderPath) + "): " + message);
    }

    // Insert shader module into cache
    shaderModules[cacheKey] = shader;

    // Return the GLuint that refers to the shader
    return shader;
}

GLuint ShaderManager::getGraphicsProgram(const std::string_view& vertexShaderPath, const std::string_view& fragmentShaderPath)
{
    // Use cached program if available
    auto cacheKey = std::make_tuple(std::string(vertexShaderPath), std::string(fragmentShaderPath));
    if (graphicsPrograms.contains(cacheKey))
    {
        return graphicsPrograms[cacheKey];
    }

    // Load the shaders
    GLuint vertexModule = getShaderModule(vertexShaderPath, GL_VERTEX_SHADER);
    GLuint fragmentModule = getShaderModule(fragmentShaderPath, GL_FRAGMENT_SHADER);

    // Create the program object and bind the two shaders to it.
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexModule);
    glAttachShader(program, fragmentModule);
    {
        // Link the program together (this is akin to compiling an individual shader)
        glLinkProgram(program);

        // Verify that program linking was successful
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

    // Insert program into cache
    graphicsPrograms[cacheKey] = program;

    return program;
}

GLuint ShaderManager::getComputeProgram(const std::string_view& computeShaderPath)
{
    // Use cached program if available
    auto cacheKey = std::string(computeShaderPath);
    if (computePrograms.contains(cacheKey))
    {
        return computePrograms[cacheKey];
    }

    // Load the compute shader
    GLuint module = getShaderModule(computeShaderPath, GL_COMPUTE_SHADER);

    // Create the compute program
    GLuint program = glCreateProgram();
    glAttachShader(program, module);
    {
        // Link the compute program
        glLinkProgram(program);

        // Verify that linking was successfull
        GLint isSuccess;
        glGetProgramiv(program, GL_LINK_STATUS, &isSuccess);

        if (!isSuccess)
        {
            GLint messageLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &messageLength);

            std::string message {};
            message.resize(messageLength);

            glGetProgramInfoLog(program, message.size(), nullptr, message.data());

            std::cout << message.data() << std::endl;
            throw std::runtime_error("Failed to link shader program: " + message);
        }
    }
    glDetachShader(program, module);

    // Insert program into cache
    computePrograms[cacheKey] = program;

    return program;
}
