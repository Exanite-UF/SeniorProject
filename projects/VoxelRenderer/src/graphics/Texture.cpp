#include "Texture.h"

#include <src/graphics/TextureManager.h>

Texture::Texture(GLuint textureId, TextureType type, glm::ivec2 size, bool isCubemap)
    : size(size)
{
    this->textureId = textureId;
    if (TextureManager::getInstance().areBindlessTexturesEnabled())
    {
        // bindlessHandle = glGetTextureHandleARB(textureId);
    }

    this->type = type;
    _isCubemap = isCubemap;
}

GLuint Texture::getTextureId()
{
    return textureId;
}

uint64_t Texture::getBindlessHandle()
{

    return bindlessHandle;
}

glm::ivec2 Texture::getSize()
{
    return size;
}

TextureType Texture::getType()
{
    return type;
}

bool Texture::isCubemap() const
{
    return _isCubemap;
}

void Texture::makeBindlessHandle()
{
    // Make the bindles handle if needed
    if (bindlessHandle == 0)
    {
        // bindlessHandle = glGetTextureHandleARB(textureId);
    }

    // Make the handle resident on whatever thread is calling this function
    // if (!glIsTextureHandleResidentARB(bindlessHandle))
    //{
    //    glMakeTextureHandleResidentARB(bindlessHandle);
    //}
}
