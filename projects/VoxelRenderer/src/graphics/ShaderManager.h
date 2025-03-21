#pragma once

#include <src/utilities/OpenGl.h>

#include <string>
#include <tuple>
#include <unordered_map>

#include <src/utilities/Singleton.h>
#include <src/utilities/TupleHasher.h>

class ShaderManager : public Singleton<ShaderManager>
{
    friend class Singleton;

private:
    // (shaderPath, shaderType) -> (shaderModule)
    std::unordered_map<std::tuple<std::string, GLenum>, GLuint, TupleHasher<std::tuple<std::string, GLenum>>> shaderModules;

    // (vertexShaderPath, fragmentShaderPath) -> (shaderProgram)
    std::unordered_map<std::tuple<std::string, std::string>, GLuint, TupleHasher<std::tuple<std::string, std::string>>> graphicsPrograms;

    // (computeShaderPath) -> (shaderProgram)
    std::unordered_map<std::string, GLuint> computePrograms {};

    // Loads and compiles a shader file into a shader module
    // Output is cached
    GLuint getShaderModule(const std::string_view& shaderPath, GLenum shaderType);

    ~ShaderManager();

public:
    // Loads, compiles, and links vertex and fragment shaders into a graphics program
    // Output is cached
    GLuint getGraphicsProgram(const std::string_view& vertexShaderPath, const std::string_view& fragmentShaderPath);

    // Loads, compiles, and links vertex and fragment shaders into a graphics program
    // Output is cached
    // Forces ScreenTri.vertex.glsl to be used as the vertex shader
    GLuint getPostProcessProgram(const std::string_view& fragmentShaderPath);

    // Loads, compiles, and links a compute shader into a compute program
    // Output is cached
    GLuint getComputeProgram(const std::string_view& computeShaderPath);
};
