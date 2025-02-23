#pragma once

#include "TextureType.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

#include <src/utilities/NonCopyable.h>

class Texture : public NonCopyable
{
private:
    GLuint textureId;
    GLuint64 bindlessHandle;

    glm::ivec2 size;

    TextureType type;

public:
    explicit Texture(GLuint textureId, TextureType type, glm::ivec2 size);

    GLuint getTextureId();
    uint64_t getBindlessHandle();

    glm::ivec2 getSize();

    TextureType getType();
};
