#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <unordered_map>

#include <src/graphics/Texture.h>
#include <src/graphics/TextureType.h>
#include <src/utilities/TupleHasher.h>

class TextureManager
{
private:
    // (path, format) -> texture
    std::unordered_map<std::tuple<std::string_view, GLenum>, std::shared_ptr<Texture>, TupleHasher<std::tuple<std::string_view, GLenum>>> textures;

    static GLenum getOpenGlFormat(TextureType type);
    static int getFormatChannelCount(GLenum format);

    std::shared_ptr<Texture> loadTexture(std::string_view path, TextureType type, GLenum format);

    static TextureManager* instance;
    TextureManager();

public:
    // Loads a texture using a texture type preset
    std::shared_ptr<Texture> loadTexture(std::string_view path, TextureType type);

    // Loads a texture with the specified format and colorspace
    std::shared_ptr<Texture> loadTexture(std::string_view path, GLenum format);

    static TextureManager& getInstance();
};
