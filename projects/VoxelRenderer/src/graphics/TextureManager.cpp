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

int TextureManager::getFormatChannelCount(GLenum format)
{
    switch (format)
    {
        case GL_SRGB8:
            return 3;
        case GL_SRGB8_ALPHA8:
            return 4;
        case GL_RGB8:
            return 3;
        default:
            break;
    }

    throw std::runtime_error("Unsupported texture format: " + format);
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
    int width, height, rawChannelCount;
    auto rawTextureData = stbi_load(path.data(), &width, &height, &rawChannelCount, 3);
    Program::assertIsTrue(rawTextureData != nullptr, "Failed to load texture: " + std::string(path));

    try
    {
        // Rewrite data to have required number of channels
        auto requiredChannelCount = getFormatChannelCount(format);
        std::vector<uint8_t> textureData(width * height * requiredChannelCount);

        for (int entryI = 0; entryI < width * height; entryI++)
        {
            for (int channelI = 0; channelI < requiredChannelCount; channelI++)
            {
                textureData[entryI * requiredChannelCount + channelI] = rawTextureData[entryI * rawChannelCount + channelI];
            }
        }

        // Create OpenGL texture
        GLuint textureId;
        glGenTextures(1, &textureId);

        glBindTexture(GL_TEXTURE_2D, textureId);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, textureData.data());
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
        stbi_image_free(rawTextureData);

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
