#include "AsynchronousReprojection.h"

#include <src/graphics/ShaderManager.h>
#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>

#include <iostream>

GLuint AsynchronousReprojection::renderProgram;

void AsynchronousReprojection::generateMesh()
{
    vertices.resize(size.x * size.y * 3);
    indices.resize((size.x - 1) * (size.y - 1) * 2 * 3);//number of squares -> number of triangle -> number of edges

    for(int x = 0; x < size.x; x++){
        for(int y = 0; y < size.y; y++){
            std::size_t index = 3 * (x + y * size.x);

            vertices[index + 0] = (float)x / (size.x - 1);
            vertices[index + 1] = (float)y / (size.y - 1);
            vertices[index + 2] = 0;
        }
    }

    for(int x = 0; x < size.x - 1; x++){
        for(int y = 0; y < size.y - 1; y++){
            std::size_t index = 6 * (x + y * (size.x - 1));
            
            //This happens once per square
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

    glGenVertexArrays(1, &VAO);  
    glGenBuffers(1, &VBO);  
    glGenBuffers(1, &EBO);



    glGenFramebuffers(1, &framebufferId);
    setSize(size);
}

GLuint AsynchronousReprojection::getFrameBufferId() const
{
    return framebufferId;
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

    this->size = size;

    generateMesh();

    // Create color texture
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

    // Create position texture
    glGenTextures(1, &positionTextureId);
    glBindTexture(GL_TEXTURE_2D, positionTextureId);
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

    // Bind textures to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTextureId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, positionTextureId, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void AsynchronousReprojection::render(const Camera& camera)
{
    glUseProgram(renderProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTextureId);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, positionTextureId);

    glm::vec3 deltaPos = camera.transform.getGlobalPosition() - lastCameraPosition;
    glm::quat deltaRot = camera.transform.getGlobalRotation();// * glm::inverse(lastCameraRotation);

    //std::cout << deltaPos.x << " " << deltaPos.y << " " << deltaPos.z << std::endl;

    glUniform3f(glGetUniformLocation(renderProgram, "cameraPosition"), deltaPos.x, deltaPos.y, deltaPos.z);
    glUniform4f(glGetUniformLocation(renderProgram, "inverseCameraRotation"), deltaRot.x, deltaRot.y, deltaRot.z, -deltaRot.w);
    glUniform2i(glGetUniformLocation(renderProgram, "resolution"), size.x, size.y);

    //glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
    glBindVertexArray(VAO);
    {
        //glBindBuffer(GL_ARRAY_BUFFER, VBO);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);
        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    glBindVertexArray(0);

    glUseProgram(0);
}

void AsynchronousReprojection::recordCameraTransform(const Camera& camera)
{
    lastCameraPosition = camera.transform.getGlobalPosition();
    lastCameraRotation = camera.transform.getGlobalRotation();
}
