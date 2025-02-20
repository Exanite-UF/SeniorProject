#include "Texture.h"

Texture::Texture(GLuint textureId, TextureType type)
{
    this->textureId = textureId;
    bindlessHandle = glGetTextureHandleARB(textureId);

    this->type = type;
}

TextureType Texture::getType()
{
    return type;
}
