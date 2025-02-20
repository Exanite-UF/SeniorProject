#include "Texture.h"

Texture::Texture(GLuint textureId)
{
    this->textureId = textureId;
    bindlessHandle = glGetTextureHandleARB(textureId);
}
