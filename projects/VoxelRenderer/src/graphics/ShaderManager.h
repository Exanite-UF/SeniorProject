#pragma once

#include <memory>
#include <src/utilities/OpenGl.h>

#include <src/graphics/ShaderProgram.h>
#include <src/utilities/Singleton.h>
#include <src/utilities/TupleHasher.h>
#include <string>
#include <tuple>
#include <unordered_map>

class ShaderManager : public Singleton<ShaderManager>
{
    friend class Singleton;

private:
    // (shaderPath, shaderType) -> (shaderModule)
    std::unordered_map<std::tuple<std::string, GLenum>, GLuint, TupleHasher<std::tuple<std::string, GLenum>>> shaderModules;

    // (vertexShaderPath, fragmentShaderPath) -> (shaderProgram)
    std::unordered_map<std::tuple<std::string, std::string>, std::shared_ptr<ShaderProgram>, TupleHasher<std::tuple<std::string, std::string>>> graphicsPrograms;

    // (computeShaderPath) -> (shaderProgram)
    std::unordered_map<std::string, std::shared_ptr<ShaderProgram>> computePrograms {};

    // Loads and compiles a shader file into a shader module
    // Output is cached
    GLuint getShaderModule(const std::string_view& shaderPath, GLenum shaderType);

public:
    // Loads, compiles, and links vertex and fragment shaders into a graphics program
    // Output is cached
    std::shared_ptr<ShaderProgram> getGraphicsProgram(const std::string_view& vertexShaderPath, const std::string_view& fragmentShaderPath);

    // Loads, compiles, and links vertex and fragment shaders into a graphics program
    // Output is cached
    // Forces ScreenTri.vertex.glsl to be used as the vertex shader
    std::shared_ptr<ShaderProgram> getPostProcessProgram(const std::string_view& fragmentShaderPath);

    // Loads, compiles, and links a compute shader into a compute program
    // Output is cached
    std::shared_ptr<ShaderProgram> getComputeProgram(const std::string_view& computeShaderPath);
};
