#include "AsynchronousReprojection.h"

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>

#include <iostream>

GLuint AsyncReprojectionRenderer::renderProgram {};
GLuint AsyncReprojectionRenderer::combineProgram {};
GLuint AsyncReprojectionRenderer::combine2Program {};
GLuint AsyncReprojectionRenderer::denoiseProgram {};


void AsyncReprojectionRenderer::generateMesh(const glm::ivec2& size)
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

AsyncReprojectionRenderer::AsyncReprojectionRenderer()
{
    renderProgram = ShaderManager::getInstance().getGraphicsProgram(Content::renderReprojectionVertexShader, Content::renderReprojectionFragmentShader);
    combineProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::combineReprojectionFragmentShader);
    combine2Program = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::combineReprojection2FragmentShader);
    denoiseProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::denoiseFragmentShader);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
}

void AsyncReprojectionRenderer::setSize(glm::ivec2 size)
{
    if (this->size == size)
    {
        return;
    }

    this->size = size;

    // Create temp color texture
    glDeleteTextures(1, &tempColorTexture);
    glGenTextures(1, &tempColorTexture);
    glBindTexture(GL_TEXTURE_2D, tempColorTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->size.x, this->size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    generateMesh(size);

    // Create temp variance texture
    glDeleteTextures(1, &tempVarianceTexture);
    glGenTextures(1, &tempVarianceTexture);
    glBindTexture(GL_TEXTURE_2D, tempVarianceTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->size.x, this->size.y, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    generateMesh(size);
}

void AsyncReprojectionRenderer::render(GLuint framebuffer, const glm::ivec2& reprojectionResolution, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV,
    const GLuint& colorTexture, const GLuint& positionTexture, const GLuint& normalTexture, const GLuint& miscTexture)
{
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLenum drawBuffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
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

void AsyncReprojectionRenderer::combineBuffers(const GLuint& latestColorTexture, const GLuint& oldColorTexture, const GLuint& newMiscTexture, const GLuint& newColorTexture, const GLuint& oldColorSquaredTexture, const GLuint& newColorSquaredTexture,
    const GLuint& oldPositionTexture, const GLuint& newPositionTexture, const GLuint& normalTexture, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, const GLuint& varianceTexture)
{
    // This runs in the offscreen context

    // This is where combination occurs

    // Combine 1
    {
        glUseProgram(combineProgram);

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, newColorSquaredTexture, 0);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oldColorTexture);

        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, newColorTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, latestColorTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, newMiscTexture);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, oldPositionTexture);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, newPositionTexture);


        glUniform2i(glGetUniformLocation(combineProgram, "resolution"), size.x, size.y);

        
       
        {
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, buffers);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);

            glDeleteVertexArrays(1, &emptyVertexArray);
        }
        


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

    //glMemoryBarrier(GL_ALL_BARRIER_BITS);
    // Combine 2
    {
        glUseProgram(combine2Program);

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, newColorTexture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, varianceTexture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, latestColorTexture, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tempColorTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, newColorSquaredTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, newMiscTexture);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, newPositionTexture);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, normalTexture);


        glUniform3fv(glGetUniformLocation(combine2Program, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
        glUniform4f(glGetUniformLocation(combine2Program, "cameraRotation"), cameraRotation.x, cameraRotation.y, cameraRotation.z, cameraRotation.w);
        glUniform1f(glGetUniformLocation(combine2Program, "horizontalFovTan"), std::tan(cameraFOV * 0.5));

        glUniform2i(glGetUniformLocation(combine2Program, "resolution"), size.x, size.y);

        
       
        {
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(3, buffers);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);

            glDeleteVertexArrays(1, &emptyVertexArray);
        }
        


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

        glUseProgram(0);
    }

    glFinish();
    currentBuffer++;
}

void AsyncReprojectionRenderer::denoise(const GLuint& colorTexture, const GLuint& varianceTexture, const GLuint& positionTexture, const GLuint& normalTexture, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV)
{
    
    for(int i = 0; i < 2; i++){
        _denoise(2 * i, colorTexture, varianceTexture, tempColorTexture, tempVarianceTexture, positionTexture, normalTexture, cameraPosition, cameraRotation, cameraFOV);
        _denoise(2 * i + 1, tempColorTexture, tempVarianceTexture, colorTexture, varianceTexture, positionTexture, normalTexture, cameraPosition, cameraRotation, cameraFOV);
    }
}

void AsyncReprojectionRenderer::_denoise(int iteration, const GLuint& colorTexture, const GLuint& varianceTexture, const GLuint& outputColorTexture, const GLuint& outputVarianceTexture, const GLuint& positionTexture, const GLuint& normalTexture, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV)
{
    {
        glUseProgram(denoiseProgram);

        // Need to render to a custom framebuffer, that switches its render target every time
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        // Bind new data
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputColorTexture, 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, outputVarianceTexture, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, varianceTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, positionTexture);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, normalTexture);


        glUniform3fv(glGetUniformLocation(denoiseProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
        glUniform4f(glGetUniformLocation(denoiseProgram, "cameraRotation"), cameraRotation.x, cameraRotation.y, cameraRotation.z, cameraRotation.w);
        glUniform1f(glGetUniformLocation(denoiseProgram, "horizontalFovTan"), std::tan(cameraFOV * 0.5));

        glUniform1i(glGetUniformLocation(denoiseProgram, "iteration"), iteration);

        glUniform2i(glGetUniformLocation(denoiseProgram, "resolution"), size.x, size.y);

        
       
        {
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
            glDrawBuffers(2, buffers);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);

            glDeleteVertexArrays(1, &emptyVertexArray);
        }
        


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

    glFinish();
    currentBuffer++;
}
