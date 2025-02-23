#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>

class Framebuffer
{
private:
    glm::ivec2 size;

public:
    GLuint framebufferId;
    GLuint colorTextureId;
    GLuint depthTextureId;

    Framebuffer(glm::ivec2 size);

    glm::ivec2 getSize();
    void setSize(glm::ivec2 size);

    void bind();
    void unbind();
};
