#include "Texture.h"

Texture::Texture(GLuint textureId, TextureType type)
{
    this->textureId = textureId;
    // bindlessHandle = glGetTextureHandleARB(textureId);
    // glMakeTextureHandleResidentARB(bindlessHandle);

    this->type = type;
}

GLuint Texture::getTextureId()
{
    return textureId;
}

uint64_t Texture::getBindlessHandle()
{
    return bindlessHandle;
}

TextureType Texture::getType()
{
    return type;
}
