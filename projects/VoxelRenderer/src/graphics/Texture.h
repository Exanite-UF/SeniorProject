#pragma once

#include "TextureType.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <src/utilities/NonCopyable.h>

class Texture : public NonCopyable
{
private:
    GLuint textureId;
    GLuint64 bindlessHandle;

    TextureType type;

public:
    explicit Texture(GLuint textureId, TextureType type);

    GLuint getTextureId();
    uint64_t getBindlessHandle();

    TextureType getType();
};
