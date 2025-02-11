#include <fstream>
#include <src/graphics/ShaderManager.h>
#include <sstream>
#include <stdexcept>
#include <string>

ShaderManager* ShaderManager::instance = nullptr;

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

GLuint ShaderManager::getShaderModule(const std::string& shaderPath, GLenum shaderType)
{
    // Use cached shader module if available
    auto cacheKey = std::make_tuple(shaderPath, shaderType);
    if (shaderModules.contains(cacheKey))
    {
        return shaderModules[cacheKey];
    }

    // Load the shader file
    std::ifstream file(shaderPath);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open file: " + shaderPath);
    }

    // Read the shader file contents
    std::stringstream buffer {};
    buffer << file.rdbuf();

    std::string string = buffer.str();
    auto data = string.data();
    // We now have the shader file contents

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

        throw std::runtime_error("Failed to compile shader (" + shaderPath + "): " + message);
    }

    // Insert shader module into cache
    shaderModules[cacheKey] = shader;

    // Return the GLuint that refers to the shader
    return shader;
}

GLuint ShaderManager::getGraphicsProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath)
{
    // Use cached program if available
    auto cacheKey = std::make_tuple(vertexShaderPath, fragmentShaderPath);
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

GLuint ShaderManager::getComputeProgram(const std::string& computeShaderPath)
{
    // Use cached program if available
    auto cacheKey = computeShaderPath;
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

            throw std::runtime_error("Failed to link shader program: " + message);
        }
    }
    glDetachShader(program, module);

    // Insert program into cache
    computePrograms[cacheKey] = program;

    return program;
}

ShaderManager& ShaderManager::getManager()
{
    if (instance == nullptr)
    {
        instance = new ShaderManager();
    }

    return *instance;
}
