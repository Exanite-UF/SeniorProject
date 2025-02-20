#include "TextureManager.h"

#include <src/Program.h>
#include <stb_image.h>

std::shared_ptr<Texture> TextureManager::loadTexture(std::string_view path, GLenum format)
{
    int width, height, channels;
    auto textureData = stbi_load(path.data(), &width, &height, &channels, 0);
    Program::assertIsTrue(textureData != nullptr, "Failed to load texture: " + std::string(path));

    try
    {
        GLuint textureId;
        glGenTextures(1, &textureId);

        glBindTexture(GL_TEXTURE_2D, textureId);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        return std::make_shared<Texture>(textureId);
    }
    catch (...)
    {
        stbi_image_free(textureData);

        throw;
    }
}

std::shared_ptr<Texture> TextureManager::loadColorTexture(std::string_view path)
{
    return loadTexture(path, GL_RGBA);
}

std::shared_ptr<Texture> TextureManager::loadRawTexture(std::string_view path)
{
    return loadTexture(path, GL_RGBA);
}
