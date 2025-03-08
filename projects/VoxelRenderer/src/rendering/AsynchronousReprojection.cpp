#include "AsynchronousReprojection.h"

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>

#include <iostream>

GLuint AsynchronousReprojection::renderProgram;
GLuint AsynchronousReprojection::combineProgram;
GLuint AsynchronousReprojection::combineMaskProgram;

void AsynchronousReprojection::generateMesh()
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

AsynchronousReprojection::AsynchronousReprojection(glm::ivec2 size)
{
    renderProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::renderReprojectionFragmentShader);
    combineProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::combineReprojectionFragmentShader);
    combineMaskProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::makeCombineMaskFragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    setSize(size);
}

GLuint AsynchronousReprojection::getColorTexture() const
{
    if (currentFrameBuffer % 2 == 0)
    {
        return colorTextureId1;
    }
    else
    {
        return colorTextureId2;
    }
}

GLuint AsynchronousReprojection::getPositionTexture() const
{
    if (currentFrameBuffer % 2 == 0)
    {
        return positionTextureId1;
    }
    else
    {
        return positionTextureId2;
    }
}

GLuint AsynchronousReprojection::getMaterialTexture() const
{
    if (currentFrameBuffer % 2 == 0)
    {
        return materialTextureId1;
    }
    else
    {
        return materialTextureId2;
    }
}

glm::ivec2 AsynchronousReprojection::getSize()
{
    return size;
}

void AsynchronousReprojection::setSize(glm::ivec2 size)
{
    if (this->size == size)
    {
        return;
    }

    glFinish();

    this->size = size;

    generateMesh();

    // Create textures 1
    {
        // Create color texture
        glDeleteTextures(1, &colorTextureId1);
        glGenTextures(1, &colorTextureId1);
        glBindTexture(GL_TEXTURE_2D, colorTextureId1);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create position texture
        glDeleteTextures(1, &positionTextureId1);
        glGenTextures(1, &positionTextureId1);
        glBindTexture(GL_TEXTURE_2D, positionTextureId1);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, size.x, size.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create material texture
        glDeleteTextures(1, &materialTextureId1);
        glGenTextures(1, &materialTextureId1);
        glBindTexture(GL_TEXTURE_2D, materialTextureId1);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, size.x, size.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create textures 2
    {
        // Create color texture
        glDeleteTextures(1, &colorTextureId2);
        glGenTextures(1, &colorTextureId2);
        glBindTexture(GL_TEXTURE_2D, colorTextureId2);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create position texture
        glDeleteTextures(1, &positionTextureId2);
        glGenTextures(1, &positionTextureId2);
        glBindTexture(GL_TEXTURE_2D, positionTextureId2);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, size.x, size.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create material texture
        glDeleteTextures(1, &materialTextureId2);
        glGenTextures(1, &materialTextureId2);
        glBindTexture(GL_TEXTURE_2D, materialTextureId2);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, size.x, size.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create frame count texture
    glDeleteTextures(1, &frameCountTextureID1);
    glGenTextures(1, &frameCountTextureID1);
    glBindTexture(GL_TEXTURE_2D, frameCountTextureID1);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, size.x, size.y, 0, GL_RG, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteTextures(1, &frameCountTextureID2);
    glGenTextures(1, &frameCountTextureID2);
    glBindTexture(GL_TEXTURE_2D, frameCountTextureID2);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, size.x, size.y, 0, GL_RG, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Make combine mask texture
    glDeleteTextures(1, &combineMaskTextureID);
    glGenTextures(1, &combineMaskTextureID);
    glBindTexture(GL_TEXTURE_2D, combineMaskTextureID);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, size.x, size.y, 0, GL_RG, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void AsynchronousReprojection::render(GameObject& camera)
{
    glUseProgram(renderProgram);

    // std::cout << currentFrameBuffer << std::endl;

    // These use the opposite buffer that the get function returns
    if (currentFrameBuffer % 2 == 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTextureId2);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, positionTextureId2);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTextureId1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, positionTextureId1);
    }

    glm::vec3 deltaPos = camera.getTransform()->getGlobalPosition(); // - lastCameraPosition;
    glm::quat deltaRot = camera.getTransform()->getGlobalRotation(); // * glm::inverse(lastCameraRotation);

    // std::cout << deltaPos.x << " " << deltaPos.y << " " << deltaPos.z << std::endl;

    glUniform3f(glGetUniformLocation(renderProgram, "cameraPosition"), deltaPos.x, deltaPos.y, deltaPos.z);
    glUniform4f(glGetUniformLocation(renderProgram, "inverseCameraRotation"), deltaRot.x, deltaRot.y, deltaRot.z, -deltaRot.w);
    glUniform2i(glGetUniformLocation(renderProgram, "resolution"), size.x, size.y);
    glUniform1f(glGetUniformLocation(renderProgram, "horizontalFovTan"), camera.getComponent<Camera>()->getHorizontalFov());

    glBindVertexArray(VAO);
    {
        // glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        // glPointSize(3);//I don't know why the points need to be this large
        // glDrawArrays(GL_POINTS, 0, vertices.size());
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        // glBindBuffer(GL_ARRAY_BUFFER, 0);
        // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glUseProgram(0);
}

void AsynchronousReprojection::recordCameraTransform(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV)
{
    lastCameraPosition = cameraPosition;
    lastCameraRotation = cameraRotation;
    lastCameraFOV = cameraFOV;
}

void AsynchronousReprojection::swapBuffers()
{
    currentFrameBuffer++;
}

void AsynchronousReprojection::combineBuffers()
{
    static int oldBuffer = 0;
    if (oldBuffer == currentFrameBuffer)
    {
        return;
    }
    // This is where combination occurs

    // Make mask
    {
        glUseProgram(combineMaskProgram);

        if (currentFrameBuffer % 2 == 1)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorTextureId2);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, positionTextureId2);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorTextureId1);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, positionTextureId1);
        }

        glm::vec3 deltaPos = lastCameraPosition; // camera.transform.getGlobalPosition();// - lastCameraPosition;
        glm::quat deltaRot = lastCameraRotation; // camera.transform.getGlobalRotation();// * glm::inverse(lastCameraRotation);

        // glm::vec3 deltaPos = camera.transform.getGlobalPosition();// - lastCameraPosition;
        // glm::quat deltaRot = camera.transform.getGlobalRotation();// * glm::inverse(lastCameraRotation);

        glUniform3f(glGetUniformLocation(combineMaskProgram, "cameraPosition"), deltaPos.x, deltaPos.y, deltaPos.z);
        glUniform4f(glGetUniformLocation(combineMaskProgram, "inverseCameraRotation"), deltaRot.x, deltaRot.y, deltaRot.z, -deltaRot.w);
        glUniform2i(glGetUniformLocation(combineMaskProgram, "resolution"), size.x, size.y);
        glUniform1f(glGetUniformLocation(combineMaskProgram, "horizontalFovTan"), lastCameraFOV);

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, combineMaskTextureID, 0);

        glBindVertexArray(VAO);
        {
            // glBindBuffer(GL_ARRAY_BUFFER, VBO);
            // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glPointSize(3); // I don't know why the points need to be this large
            glDrawArrays(GL_POINTS, 0, vertices.size());

            // glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

            // glBindBuffer(GL_ARRAY_BUFFER, 0);
            // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);
        // glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    // Combine
    {
        glUseProgram(combineProgram);

        // Bind old data
        if (currentFrameBuffer % 2 == 1)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorTextureId2);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, positionTextureId2);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorTextureId1);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, positionTextureId1);
        }

        // Bind new data
        if (currentFrameBuffer % 2 == 1)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, positionTextureId1); // Used to reject old data

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, materialTextureId1); // Used to reject old data
        }
        else
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, positionTextureId2); // Used to reject old data

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, materialTextureId2); // Used to reject old data
        }

        // Old frame count
        if (currentFrameBuffer % 2 == 1)
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, frameCountTextureID2);
        }
        else
        {
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, frameCountTextureID1);
        }

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, combineMaskTextureID);

        // Set uniform data
        glm::vec3 deltaPos = lastCameraPosition; // camera.transform.getGlobalPosition();// - lastCameraPosition;
        glm::quat deltaRot = lastCameraRotation; // camera.transform.getGlobalRotation();// * glm::inverse(lastCameraRotation);
        // glm::vec3 deltaPos = camera.transform.getGlobalPosition();// - lastCameraPosition;
        // glm::quat deltaRot = camera.transform.getGlobalRotation();// * glm::inverse(lastCameraRotation);

        glUniform3f(glGetUniformLocation(combineProgram, "cameraPosition"), deltaPos.x, deltaPos.y, deltaPos.z);
        glUniform4f(glGetUniformLocation(combineProgram, "inverseCameraRotation"), deltaRot.x, deltaRot.y, deltaRot.z, -deltaRot.w);
        glUniform2i(glGetUniformLocation(combineProgram, "resolution"), size.x, size.y);

        glUniform1f(glGetUniformLocation(combineProgram, "horizontalFovTan"), lastCameraFOV);

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        if (currentFrameBuffer % 2 == 1)
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTextureId1, 0);
        }
        else
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTextureId2, 0);
        }

        // New frame count
        if (currentFrameBuffer % 2 == 1)
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, frameCountTextureID1, 0);
        }
        else
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, frameCountTextureID2, 0);
        }

        glBindVertexArray(VAO);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        {
            // glBindBuffer(GL_ARRAY_BUFFER, VBO);
            // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, buffers);
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

            // glBindBuffer(GL_ARRAY_BUFFER, 0);
            // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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

    glFinish();
    oldBuffer = currentFrameBuffer;
}
