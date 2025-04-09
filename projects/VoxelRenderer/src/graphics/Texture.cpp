#include "Texture.h"

Texture::Texture(GLuint textureId, TextureType type, glm::ivec2 size, bool isCubemap)
    : size(size)
{
    this->textureId = textureId;
    bindlessHandle = glGetTextureHandleARB(textureId);
    glMakeTextureHandleResidentARB(bindlessHandle);

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
