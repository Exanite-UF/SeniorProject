#pragma once

#include "TupleHasher.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <tuple>
#include <unordered_map>

class ShaderManager
{
private:
    // (shaderPath, GLenum) -> (shaderModule)
    std::unordered_map<std::tuple<std::string, GLenum>, GLuint, TupleHasher<std::tuple<std::string, GLenum>>> shaderModules;

    // (vertexShaderPath, fragmentShaderPath, type) -> (shaderProgram)
    std::unordered_map<std::tuple<std::string, std::string>, GLuint, TupleHasher<std::tuple<std::string, std::string>>> graphicsPrograms;

    // (computeShaderPath) -> (shaderProgram)
    std::unordered_map<std::string, GLuint> computePrograms {};

public:
    ShaderManager();
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;

    ~ShaderManager();

    // Loads and compiles a shader file into a shader module
    // Output is cached
    GLuint getShaderModule(const std::string& shaderPath, GLenum type);

    // Loads, compiles, and links vertex and fragment shaders into a graphics program
    // Output is cached
    GLuint getGraphicsProgram(const std::string& vertexShaderPath, const std::string& fragmentShaderPath);

    // Loads, compiles, and links a compute shader into a compute program
    // Output is cached
    GLuint getComputeProgram(const std::string& computeShaderPath);
};
