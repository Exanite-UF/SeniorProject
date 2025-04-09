#include "TextureManager.h"

#define STB_IMAGE_IMPLEMENTATION

#include <fstream>
#include <src/Program.h>
#include <src/utilities/Assert.h>
#include <stb_image.h>
#include <stdexcept>

#include <iostream>

GLenum TextureManager::getOpenGlStorageFormat(TextureType type)
{
    switch (type)
    {
        case ColorOnly:
            return GL_SRGB8;
        case ColorAlpha:
            return GL_SRGB8_ALPHA8;
        case Normal:
            return GL_RG8;
        default:
            break;
    }

    throw std::runtime_error("Unsupported texture type: " + std::to_string(type));
}

int TextureManager::getFormatChannelCount(GLenum format)
{
    switch (format)
    {
        case GL_SRGB8:
            return 3;
        case GL_SRGB8_ALPHA8:
            return 4;
        case GL_RG8:
            return 2;
        default:
            break;
    }

    throw std::runtime_error("Unsupported texture format: " + std::to_string(format));
}

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, TextureType type, GLenum storageFormat)
{
    // Use cached texture if available
    auto cacheKey = std::make_tuple(std::string(path), storageFormat);
    if (textures.contains(cacheKey))
    {
        return textures[cacheKey];
    }

    // Load texture data
    int width, height, rawChannelCount;
    auto rawTextureData = stbi_load(path.data(), &width, &height, &rawChannelCount, 3);
    Assert::isTrue(rawTextureData != nullptr, "Failed to load texture: " + std::string(path));

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

            // internalFormat (called storageFormat here) is the format of the texture stored on the GPU
            // format is the input format, as loaded from the texture file
            glTexImage2D(GL_TEXTURE_2D, 0, storageFormat, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rawTextureData);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Wrap OpenGL handle with the Texture class
        auto texture = std::make_shared<Texture>(textureId, type, glm::ivec2(width, height), false);

        // Insert texture into cache
        textures[cacheKey] = texture;
        stbi_image_free(rawTextureData);

        return texture;
    }
    catch (...)
    {
        stbi_image_free(rawTextureData);

        throw;
    }
}

std::shared_ptr<Texture> TextureManager::loadCubemapTexture(std::string_view path, TextureType type, GLenum storageFormat)
{
    // Use cached texture if available
    auto cacheKey = std::make_tuple(std::string(path), storageFormat);
    if (textures.contains(cacheKey))
    {
        return textures[cacheKey];
    }

    // Load texture data
    std::ifstream indirectFile(path.data());
    if (!indirectFile.is_open())
    {
        std::cout << "Failed to load cube map indirect file: " + std::string(path) << std::endl;
    }
    Assert::isTrue(indirectFile.is_open(), "Failed to load cube map indirect file: " + std::string(path));

    std::string prePath = path.data();
    prePath = prePath.substr(0, prePath.find_last_of('/') + 1);

    // Create OpenGL texture
    GLuint textureId;
    glGenTextures(1, &textureId);

    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

    // Load the data
    int width, height, rawChannelCount;
    for (int i = 0; i < 6; i++)
    {
        // load the file name for the specific face
        std::string faceFileName;
        bool foundFileName = false;
        if (std::getline(indirectFile, faceFileName))
        {
            foundFileName = true;
        }

        Assert::isTrue(foundFileName, "Failed to file path of " + std::to_string(i) + "th cube map texture: " + std::string(path));

        faceFileName = prePath + faceFileName;
        auto rawTextureData = stbi_loadf(faceFileName.c_str(), &width, &height, &rawChannelCount, 3);
        // If the image fails to load, throw
        // But first free the memory
        try
        {
            if (rawTextureData == nullptr)
            {
                std::cout << "Failed to load texture: " + std::string(path) << std::endl;
            }
            Assert::isTrue(rawTextureData != nullptr, "Failed to load texture: " + std::string(path));
        }
        catch (...)
        {
            stbi_image_free(rawTextureData);

            throw;
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, storageFormat, width, height, 0, GL_RGB, GL_FLOAT, rawTextureData);

        stbi_image_free(rawTextureData);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Wrap OpenGL handle with the Texture class
    auto texture = std::make_shared<Texture>(textureId, type, glm::ivec2(width, height), true);

    // Insert texture into cache
    textures[cacheKey] = texture;

    return texture;
}

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, TextureType type)
{
    return loadTexture(path, type, getOpenGlStorageFormat(type));
}

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, GLenum storageFormat)
{
    return loadTexture(path, Unknown, storageFormat);
}

std::shared_ptr<Texture> TextureManager::loadCubemapTexture(std::string_view path, TextureType type)
{
    return loadCubemapTexture(path, type, getOpenGlStorageFormat(type));
}

std::shared_ptr<Texture> TextureManager::loadCubemapTexture(std::string_view path, GLenum storageFormat)
{
    return loadCubemapTexture(path, Unknown, storageFormat);
}
