#pragma once

#include "TextureType.h"

#include <glm/vec2.hpp>

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

class Texture : public NonCopyable
{
private:
    GLuint textureId = 0;
    GLuint64 bindlessHandle = 0;

    glm::ivec2 size {};

    TextureType type {};

    bool _isCubemap;

public:
    explicit Texture(GLuint textureId, TextureType type, glm::ivec2 size, bool isCubemap);

    GLuint getTextureId();
    uint64_t getBindlessHandle();

    glm::ivec2 getSize();

    TextureType getType();

    bool isCubemap() const;

    // This needs to be run on the offscreen context
    void makeBindlessHandle();
};
