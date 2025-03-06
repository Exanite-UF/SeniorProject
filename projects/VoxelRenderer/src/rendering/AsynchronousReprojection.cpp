#include "AsynchronousReprojection.h"

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>

#include <iostream>

GLuint AsynchronousReprojection::renderProgram;
GLuint AsynchronousReprojection::combineProgram;
GLuint AsynchronousReprojection::combineMaskProgram;

void AsynchronousReprojection::generateMesh(const glm::ivec2& size)
{
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

AsynchronousReprojection::AsynchronousReprojection()
{
    renderProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::renderReprojectionFragmentShader);
    combineProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::combineReprojectionFragmentShader);
    combineMaskProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::makeCombineMaskFragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

void AsynchronousReprojection::setSize(glm::ivec2 size)
{
    if (this->size == size)
    {
        return;
    }

    this->size = size;

    generateMesh(size);

    // Create mask texture
    glDeleteTextures(1, &combineMaskTextureID);
    glGenTextures(1, &combineMaskTextureID);
    glBindTexture(GL_TEXTURE_2D, combineMaskTextureID);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, this->size.x, this->size.y, 0, GL_RED, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    for (int i = 0; i < 2; i++)
    {
        // Create frame counter textures
        glDeleteTextures(1, &frameCountTextures[i]);
        glGenTextures(1, &frameCountTextures[i]);
        glBindTexture(GL_TEXTURE_2D, frameCountTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, this->size.x, this->size.y, 0, GL_RED, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void AsynchronousReprojection::render(GLuint framebuffer, const glm::ivec2& reprojectionResolution, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV,
    const GLuint& colorTexture, const GLuint& positionTexture, const GLuint& normalTexture, const GLuint& materialTexture)
{
    glUseProgram(renderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, positionTexture);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, materialTexture);

    glUniform3fv(glGetUniformLocation(renderProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
    glUniform4f(glGetUniformLocation(renderProgram, "inverseCameraRotation"), cameraRotation.x, cameraRotation.y, cameraRotation.z, -cameraRotation.w);
    glUniform2i(glGetUniformLocation(renderProgram, "resolution"), reprojectionResolution.x, reprojectionResolution.y);
    glUniform1f(glGetUniformLocation(renderProgram, "horizontalFovTan"), std::tan(cameraFOV * 0.5));

    glBindVertexArray(VAO);
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLenum drawBuffers[4] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
        glDrawBuffers(4, drawBuffers);

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);
}

void AsynchronousReprojection::combineBuffers(const glm::vec3& lastRenderedCameraPosition, const glm::quat& lastRenderedCameraRotation, const float& lastRenderedCameraFOV,
    const GLuint& oldColorTexture, const GLuint& newColorTexture, const GLuint& oldPositionTexture, const GLuint& newPositionTexture, const GLuint& newMaterialTexture)
{
    // This runs in the offscreen context

    // This is where combination occurs

    // I need to make a new VAO since they cannot be shared across contexts
    // Luckily they don't use much data
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // Make mask
    {
        glUseProgram(combineMaskProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oldPositionTexture); // Old position

        glUniform3fv(glGetUniformLocation(combineMaskProgram, "cameraPosition"), 1, glm::value_ptr(lastRenderedCameraPosition));
        glUniform4f(glGetUniformLocation(combineMaskProgram, "inverseCameraRotation"), lastRenderedCameraRotation.x, lastRenderedCameraRotation.y, lastRenderedCameraRotation.z, -lastRenderedCameraRotation.w);
        glUniform2i(glGetUniformLocation(combineMaskProgram, "resolution"), size.x, size.y);
        glUniform1f(glGetUniformLocation(combineMaskProgram, "horizontalFovTan"), std::tan(lastRenderedCameraFOV * 0.5));

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, combineMaskTextureID, 0);

        glBindVertexArray(VAO);
        {
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glPointSize(3); // I don't know why the points need to be this large
            glDrawArrays(GL_POINTS, 0, vertices.size());
        }
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);
    }

    // Combine
    {
        glUseProgram(combineProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oldPositionTexture); // Old position

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, oldColorTexture); // Old color

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, newPositionTexture); // New position

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, newMaterialTexture); // New material

        // Old frame count
        if (currentBuffer % 2 == 0)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, frameCountTextures[0]);
        }
        else
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, frameCountTextures[1]);
        }

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, combineMaskTextureID);

        glUniform3fv(glGetUniformLocation(combineProgram, "cameraPosition"), 1, glm::value_ptr(lastRenderedCameraPosition));
        glUniform4f(glGetUniformLocation(combineProgram, "inverseCameraRotation"), lastRenderedCameraRotation.x, lastRenderedCameraRotation.y, lastRenderedCameraRotation.z, -lastRenderedCameraRotation.w);
        glUniform2i(glGetUniformLocation(combineProgram, "resolution"), size.x, size.y);
        glUniform1f(glGetUniformLocation(combineProgram, "horizontalFovTan"), std::tan(lastRenderedCameraFOV * 0.5));

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, newColorTexture, 0);
        if (currentBuffer % 2 == 0)
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, frameCountTextures[1], 0);
        }
        else
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, frameCountTextures[0], 0);
        }

        glBindVertexArray(VAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        {
            const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, buffers);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }
        glDisable(GL_BLEND);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);
    }

    glDeleteVertexArrays(1, &VAO);

    glFinish();
    currentBuffer++;
}
