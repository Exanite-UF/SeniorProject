#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <memory>

#include <src/graphics/Texture.h>
#include <src/utilities/TupleHasher.h>
#include <string>
#include <unordered_map>

class TextureManager
{
private:
    std::unordered_map<std::tuple<std::string_view, GLenum>, std::shared_ptr<Texture>, TupleHasher<std::tuple<std::string_view, GLenum>>> textures;

    std::shared_ptr<Texture> loadTexture(std::string_view path, GLenum format);

public:
    // Loads a file as an RGBA8 sRGB texture (will be converted to linear 0-1 when sampling)
    // Use this for albedo
    std::shared_ptr<Texture> loadColorTexture(std::string_view path);

    // Loads a file as a RGBA8 raw texture (will be converted to 0-1 when sampling, but without colorspace conversion)
    // Use this for normals, metallic, roughness, etc
    std::shared_ptr<Texture> loadRawTexture(std::string_view path);
};
