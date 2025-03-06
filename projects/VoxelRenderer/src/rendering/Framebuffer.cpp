#include "Framebuffer.h"

Framebuffer::Framebuffer(glm::ivec2 size)
{
    glGenFramebuffers(1, &framebufferId);
    setSize(size);
}

Framebuffer::~Framebuffer()
{
    glDeleteTextures(1, &colorTextureId);
    glDeleteTextures(1, &depthTextureId);
}

glm::ivec2 Framebuffer::getSize()
{
    return size;
}

void Framebuffer::setSize(glm::ivec2 size)
{
    if (this->size == size)
    {
        return;
    }

    this->size = size;

    // Create color texture
    glDeleteTextures(1, &colorTextureId);
    glGenTextures(1, &colorTextureId);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create depth texture (this is a reverse z-buffer texture)
    glDeleteTextures(1, &depthTextureId);
    glGenTextures(1, &depthTextureId);
    glBindTexture(GL_TEXTURE_2D, depthTextureId);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind textures to framebuffer
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferId);
    {
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureId, 0);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureId, 0);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::bind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferId);
}

void Framebuffer::unbind()
{
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
