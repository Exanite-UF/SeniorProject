#include "AsynchronousReprojection.h"

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>

#include <iostream>
#include <tracy/Tracy.hpp>

GLuint AsyncReprojectionRenderer::renderProgram {};
GLuint AsyncReprojectionRenderer::bypassProgram {};

void AsyncReprojectionRenderer::generateMesh(const glm::ivec2& size)
{
    ZoneScoped;

    vertices.resize(size.x * size.y * 3);
    indices.resize((size.x - 1) * (size.y - 1) * 2 * 3); // number of squares -> number of triangle -> number of edges

    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            std::size_t index = 3 * (x + y * size.x);

            vertices[index + 0] = (float)x / (size.x - 1);
            vertices[index + 1] = (float)y / (size.y - 1);
            vertices[index + 2] = 0;
        }
    }

    for (int x = 0; x < size.x - 1; x++)
    {
        for (int y = 0; y < size.y - 1; y++)
        {
            std::size_t index = 6 * (x + y * (size.x - 1));

            // This happens once per square
            std::size_t topLeftI = (x + y * size.x);
            std::size_t topRightI = topLeftI + 1;
            std::size_t bottomLeftI = topLeftI + size.x;
            std::size_t bottomRightI = topRightI + size.x;

            indices[index + 0] = topLeftI;
            indices[index + 1] = bottomLeftI;
            indices[index + 2] = topRightI;

            indices[index + 3] = topRightI;
            indices[index + 4] = bottomLeftI;
            indices[index + 5] = bottomRightI;
        }
    }

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

AsyncReprojectionRenderer::AsyncReprojectionRenderer()
{
    renderProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::renderReprojectionFragmentShader);
    bypassProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::bypassReprojectionFragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

void AsyncReprojectionRenderer::setRenderResolution(glm::ivec2 size)
{
    ZoneScoped;

    if (this->size == size)
    {
        return;
    }

    this->size = size;

    generateMesh(size);
}

void AsyncReprojectionRenderer::render(GLuint framebuffer, const glm::ivec2& reprojectionResolution, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV,
    const GLuint& colorTexture, const GLuint& positionTexture, const GLuint& normalTexture, const GLuint& miscTexture)
{
    ZoneScoped;

    glUseProgram(renderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, miscTexture);

    glUniform3fv(glGetUniformLocation(renderProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
    glUniform4f(glGetUniformLocation(renderProgram, "inverseCameraRotation"), cameraRotation.x, cameraRotation.y, cameraRotation.z, -cameraRotation.w);
    glUniform2i(glGetUniformLocation(renderProgram, "resolution"), reprojectionResolution.x, reprojectionResolution.y);
    glUniform1f(glGetUniformLocation(renderProgram, "horizontalFovTan"), std::tan(cameraFOV * 0.5));

    glBindVertexArray(VAO);
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);
        glDepthMask(GL_TRUE);//Make the depth buffer writable
        
        GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, drawBuffers);

        glClearDepth(0); // Reverse-Z
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glDisable(GL_DEPTH_TEST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
    }
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);
}


void AsyncReprojectionRenderer::bypass(GLuint framebuffer, const glm::ivec2& reprojectionResolution, const GLuint& inputTexture)
{
    ZoneScoped;

    glUseProgram(bypassProgram);

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
        glDrawBuffers(4, drawBuffers);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);
}
