#include "VoxelRenderer.h"

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/OpenGl.h>
#include <src/utilities/TupleHasher.h>
#include <src/world/Material.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunk.h>

GLuint VoxelRenderer::prepareRayTraceFromCameraProgram;
GLuint VoxelRenderer::resetHitInfoProgram;
GLuint VoxelRenderer::resetVisualInfoProgram;
GLuint VoxelRenderer::fullCastProgram;
GLuint VoxelRenderer::pathTraceToFramebufferProgram;
GLuint VoxelRenderer::afterCastProgram;

void VoxelRenderer::remakeTextures()
{
    isSizingDirty = false;

    // This will delete the texture currently bound to this variable, and set the variable equal to 0
    // If the variable is 0, meaning that no texture is bound, then it will do nothing
    // glDeleteTextures(1, &rayStartBuffer);
    // glDeleteTextures(1, &rayDirectionBuffer);
    uint64_t size1D = size.x * size.y * raysPerPixel;

    rayHitMiscBuffer.setSize(2 * size1D);

    normalBuffer.setSize(size.x * size.y);
    positionBuffer.setSize(size.x * size.y);
    materialBuffer.setSize(size.x * size.y);

    // Create a new texture
    rayStartBuffer1.setSize(size1D);
    rayDirectionBuffer1.setSize(size1D);
    rayStartBuffer2.setSize(size1D);
    rayDirectionBuffer2.setSize(size1D);

    attentuationBuffer1.setSize(size1D);
    accumulatedLightBuffer1.setSize(size1D);
    attentuationBuffer2.setSize(size1D);
    accumulatedLightBuffer2.setSize(size1D);
}

void VoxelRenderer::handleDirtySizing()
{
    if (!isSizingDirty)
    {
        return;
    }

    remakeTextures();
}

VoxelRenderer::VoxelRenderer()
{
    prepareRayTraceFromCameraProgram = ShaderManager::getInstance().getComputeProgram(Content::prepareRayTraceFromCameraComputeShader);
    resetHitInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetHitInfoComputeShader);
    resetVisualInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetVisualInfoComputeShader);
    fullCastProgram = ShaderManager::getInstance().getComputeProgram(Content::fullCastComputeShader);
    afterCastProgram = ShaderManager::getInstance().getComputeProgram(Content::afterCastComputerShader);

    pathTraceToFramebufferProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::pathTraceToFramebufferShader);

    glGenBuffers(1, &materialTexturesBuffer); // Generate the buffer that will store the material textures
}

void VoxelRenderer::afterCast()
{
    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    // Reset the hit info
    glUseProgram(afterCastProgram);

    rayHitMiscBuffer.bind(0);
    attentuationBuffer1.bind(1);
    attentuationBuffer2.bind(2);

    glUniform3i(glGetUniformLocation(afterCastProgram, "resolution"), size.x, size.y, raysPerPixel);
    glUniform1i(glGetUniformLocation(afterCastProgram, "currentBuffer"), currentBuffer % 2 == 0);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rayHitMiscBuffer.unbind();
    attentuationBuffer1.unbind();
    attentuationBuffer2.unbind();

    glUseProgram(0);
}

void VoxelRenderer::setResolution(glm::ivec2 size)
{
    if (this->size == size)
    {
        return;
    }

    this->size = size;
    isSizingDirty = true;
}

void VoxelRenderer::setRaysPerPixel(int number)
{
    if (raysPerPixel == number)
    {
        return;
    }

    raysPerPixel = number;
    isSizingDirty = true;
}

void VoxelRenderer::prepareRayTraceFromCamera(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, bool resetLight)
{
    handleDirtySizing(); // Handle dirty sizing, this function is supposed to prepare data for rendering, as such it needs to prepare the correct amount of data

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glUseProgram(prepareRayTraceFromCameraProgram);

    if (currentBuffer % 2 == 0)
    {
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(1);
    }
    else
    {
        rayStartBuffer2.bind(0);
        rayDirectionBuffer2.bind(1);
    }

    {
        glUniform3i(glGetUniformLocation(prepareRayTraceFromCameraProgram, "resolution"), size.x, size.y, raysPerPixel);
        glUniform3fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camPosition"), 1, glm::value_ptr(cameraPosition));
        glUniform4fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camRotation"), 1, glm::value_ptr(cameraRotation));
        glUniform1f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "horizontalFovTan"), std::tan(cameraFOV * 0.5));

        glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000000) / 1000000.f, (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }

    if (currentBuffer % 2 == 0)
    {
        rayStartBuffer1.unbind();
        rayDirectionBuffer1.unbind();
    }
    else
    {
        rayStartBuffer2.unbind();
        rayDirectionBuffer2.unbind();
    }

    resetHitInfo();
    resetVisualInfo(resetLight, true, true, resetLight);
}

void VoxelRenderer::executeRayTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, bool isFirstRay)
{
    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(fullCastProgram);

    // bind rayStart info
    if (currentBuffer % 2 == 0)
    {
        rayStartBuffer1.bind(0); // Input
        rayDirectionBuffer1.bind(1); // Input
        rayStartBuffer2.bind(2); // Output
        rayDirectionBuffer2.bind(3); // Output
    }
    else
    {
        rayStartBuffer1.bind(2); // Output
        rayDirectionBuffer1.bind(3); // Output
        rayStartBuffer2.bind(0); // Input
        rayDirectionBuffer2.bind(1); // Input
    }

    // Occupancy Map = 4
    // Material Map = 5

    rayHitMiscBuffer.bind(6);

    MaterialManager& materialManager = MaterialManager::getInstance();
    materialManager.getMaterialDefinitionsBuffer().bind(7); // This binds the material definitions for each material

    if (currentBuffer % 2 == 0)
    {
        attentuationBuffer1.bind(8); // Input
        accumulatedLightBuffer1.bind(9); // Input
        attentuationBuffer2.bind(10); // Output
        accumulatedLightBuffer2.bind(11); // Output
    }
    else
    {
        attentuationBuffer1.bind(10); // Output
        accumulatedLightBuffer1.bind(11); // Output
        attentuationBuffer2.bind(8); // Input
        accumulatedLightBuffer2.bind(9); // Input
    }

    normalBuffer.bind(12);
    positionBuffer.bind(13);
    materialBuffer.bind(14);

    {
        GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (size.y + 8 - 1) / 8;
        GLuint workGroupsZ = raysPerPixel;

        glUniform3i(glGetUniformLocation(fullCastProgram, "resolution"), size.x, size.y, raysPerPixel);
        glUniform1i(glGetUniformLocation(fullCastProgram, "isFirstRay"), isFirstRay);
        glUniform1f(glGetUniformLocation(fullCastProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        for (auto& chunkComponent : chunks)
        {
            std::shared_lock lock(chunkComponent->getMutex());

            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

            auto& chunk = chunkComponent->getChunk();

            chunk->bindBuffers(4, 5);
            {
                const auto chunkSize = chunk->getSize();

                glUniform3i(glGetUniformLocation(fullCastProgram, "cellCount"), chunkSize.x / 2, chunkSize.y / 2, chunkSize.z / 2);
                glUniform1ui(glGetUniformLocation(fullCastProgram, "occupancyMapLayerCount"), chunk->getOccupancyMapIndices().size() - 2);
                glUniform1uiv(glGetUniformLocation(fullCastProgram, "occupancyMapIndices"), chunk->getOccupancyMapIndices().size() - 1, chunk->getOccupancyMapIndices().data());

                glUniform3fv(glGetUniformLocation(fullCastProgram, "voxelWorldPosition"), 1, glm::value_ptr(chunkComponent->getTransform()->getGlobalPosition()));
                glUniform4fv(glGetUniformLocation(fullCastProgram, "voxelWorldRotation"), 1, glm::value_ptr(chunkComponent->getTransform()->getGlobalRotation()));
                glUniform3fv(glGetUniformLocation(fullCastProgram, "voxelWorldScale"), 1, glm::value_ptr(chunkComponent->getTransform()->getLossyGlobalScale()));

                glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
            chunk->unbindBuffers();
        }
    }

    // unbind rayStart info
    rayStartBuffer1.unbind();
    rayDirectionBuffer1.unbind();
    rayStartBuffer2.unbind();
    rayDirectionBuffer2.unbind();

    rayHitMiscBuffer.unbind();

    materialManager.getMaterialDefinitionsBuffer().unbind();

    attentuationBuffer1.unbind();
    accumulatedLightBuffer1.unbind();
    attentuationBuffer2.unbind();
    accumulatedLightBuffer2.unbind();

    normalBuffer.unbind();
    positionBuffer.unbind();
    materialBuffer.unbind();

    glUseProgram(0);

    currentBuffer++;
}

void VoxelRenderer::executePathTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, int bounces)
{
    for (int i = 0; i <= bounces; i++)
    {
        executeRayTrace(chunks, i == 0);
        afterCast();
        resetVisualInfo(false, false, false, true);
    }
}

void VoxelRenderer::resetHitInfo()
{
    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    // Reset the hit info
    glUseProgram(resetHitInfoProgram);

    rayHitMiscBuffer.bind(0);

    glUniform3i(glGetUniformLocation(resetHitInfoProgram, "resolution"), size.x, size.y, raysPerPixel);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rayHitMiscBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::resetVisualInfo(bool resetLight, bool resetAttenuation, bool resetFirstHit, bool drawSkyBox)
{
    glUseProgram(resetVisualInfoProgram);

    // bind rayStart info
    if (currentBuffer % 2 == 0)
    {
        rayStartBuffer1.bind(0); // Input
        rayDirectionBuffer1.bind(1); // Input
    }
    else
    {
        rayStartBuffer2.bind(0); // Input
        rayDirectionBuffer2.bind(1); // Input
    }

    attentuationBuffer1.bind(2);
    accumulatedLightBuffer1.bind(3);
    attentuationBuffer2.bind(4);
    accumulatedLightBuffer2.bind(5);
    normalBuffer.bind(6);
    positionBuffer.bind(7);
    materialBuffer.bind(8);

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glUniform3i(glGetUniformLocation(resetVisualInfoProgram, "resolution"), size.x, size.y, raysPerPixel);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "resetLight"), resetLight);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "resetAttentuation"), resetAttenuation);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "resetFirstHit"), resetFirstHit);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "drawSkybox"), drawSkyBox);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "currentBuffer"), currentBuffer % 2 == 0);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    if (currentBuffer % 2 == 0)
    {
        rayStartBuffer1.unbind();
        rayDirectionBuffer1.unbind();
    }
    else
    {
        rayStartBuffer2.unbind();
        rayDirectionBuffer2.unbind();
    }

    attentuationBuffer1.unbind();
    accumulatedLightBuffer1.unbind();
    attentuationBuffer2.unbind();
    accumulatedLightBuffer2.unbind();
    normalBuffer.unbind();
    positionBuffer.unbind();
    materialBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::render(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV)
{
    glUseProgram(pathTraceToFramebufferProgram);

    if (currentBuffer % 2 == 0)
    {
        accumulatedLightBuffer1.bind(0);
    }
    else
    {
        accumulatedLightBuffer2.bind(0);
    }

    normalBuffer.bind(1);
    positionBuffer.bind(2);
    materialBuffer.bind(3);

    glUniform3i(glGetUniformLocation(pathTraceToFramebufferProgram, "resolution"), size.x, size.y, raysPerPixel);

    glUniform4fv(glGetUniformLocation(pathTraceToFramebufferProgram, "cameraRotation"), 1, glm::value_ptr(cameraRotation));
    glUniform3fv(glGetUniformLocation(pathTraceToFramebufferProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDepthFunc(GL_ALWAYS);
    {
        glDrawBuffers(4, drawBuffers.data());

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (currentBuffer % 2 == 0)
    {
        accumulatedLightBuffer1.unbind();
    }
    else
    {
        accumulatedLightBuffer2.unbind();
    }

    normalBuffer.unbind();
    positionBuffer.unbind();
    materialBuffer.unbind();

    glUseProgram(0);
}
