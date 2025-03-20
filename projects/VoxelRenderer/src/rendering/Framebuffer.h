#pragma once

#include <glm/vec2.hpp>

#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

class Framebuffer : public NonCopyable
{
private:
    glm::ivec2 size {};

public:
    GLuint framebufferId = 0;
    GLuint colorTextureId = 0;
    GLuint depthTextureId = 0;

    Framebuffer(glm::ivec2 size);
    ~Framebuffer();

    glm::ivec2 getSize();
    void setSize(glm::ivec2 size);

    void bind();
    void unbind();
};
