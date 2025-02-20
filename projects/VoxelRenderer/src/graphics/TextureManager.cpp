#include "TextureManager.h"

#include <src/Program.h>
#include <stb_image.h>
#include <stdexcept>

GLenum TextureManager::getOpenGlFormat(TextureType type)
{
    switch (type)
    {
        case ColorOnly:
            return GL_SRGB8;
        case ColorAlpha:
            return GL_SRGB8_ALPHA8;
        case Normal:
            return GL_RGB8;
        default:
            break;
    }

    throw std::runtime_error("Unsupported texture type: " + type);
}

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, TextureType type, GLenum format)
{
    // Use cached texture if available
    auto cacheKey = std::make_tuple(std::string(path), format);
    if (textures.contains(cacheKey))
    {
        return textures[cacheKey];
    }

    // Load texture data
    int width, height, channels;
    auto textureData = stbi_load(path.data(), &width, &height, &channels, 3);
    Program::assertIsTrue(textureData != nullptr, "Failed to load texture: " + std::string(path));

    try
    {
        // Create OpenGL texture
        GLuint textureId;
        glGenTextures(1, &textureId);

        glBindTexture(GL_TEXTURE_2D, textureId);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, textureData);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Wrap OpenGL handle with the Texture class
        auto texture = std::make_shared<Texture>(textureId, type);

        // Insert texture into cache
        textures[cacheKey] = texture;

        return texture;
    }
    catch (...)
    {
        stbi_image_free(textureData);

        throw;
    }
}

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, TextureType type)
{
    return loadTexture(path, type, getOpenGlFormat(type));
}

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, GLenum format)
{
    return loadTexture(path, Unknown, format);
}
