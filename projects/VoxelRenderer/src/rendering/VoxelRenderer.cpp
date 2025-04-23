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
#include <unordered_set>

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/Log.h>
#include <src/utilities/OpenGl.h>
#include <src/utilities/TupleHasher.h>
#include <src/world/Material.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunk.h>
#include <tracy/Tracy.hpp>

GLuint VoxelRenderer::prepareRayTraceFromCameraProgram;
GLuint VoxelRenderer::resetHitInfoProgram;
GLuint VoxelRenderer::resetVisualInfoProgram;
GLuint VoxelRenderer::fullCastProgram;
GLuint VoxelRenderer::pathTraceToFramebufferProgram;
GLuint VoxelRenderer::afterCastProgram;
GLuint VoxelRenderer::resetPrimaryRayInfoProgram;
GLuint VoxelRenderer::beforeCastProgram;
GLuint VoxelRenderer::primaryRayProgram;
GLuint VoxelRenderer::groupPixelsProgram;

void VoxelRenderer::remakeTextures()
{
    ZoneScoped;

    isSizingDirty = false;

    // This will delete the texture currently bound to this variable, and set the variable equal to 0
    // If the variable is 0, meaning that no texture is bound, then it will do nothing
    // glDeleteTextures(1, &rayStartBuffer);
    // glDeleteTextures(1, &rayDirectionBuffer);
    uint64_t size1D = size.x * size.y;

    rayMisc.setSize(1 * size1D);

    normalBuffer.setSize(size1D);
    positionBuffer.setSize(size1D);
    materialUVBuffer.setSize(size1D);
    materialBuffer.setSize(size1D);
    primaryDirection.setSize(size1D);
    secondaryDirection.setSize(size1D);

    // Create a new texture
    rayStartBuffer1.setSize(size1D);
    rayDirectionBuffer1.setSize(size1D);
    rayStartBuffer2.setSize(size1D);
    rayDirectionBuffer2.setSize(size1D);

    attentuationBuffer1.setSize(size1D);
    accumulatedLightBuffer1.setSize(size1D);
    attentuationBuffer2.setSize(size1D);
    accumulatedLightBuffer2.setSize(size1D);

    sampleDirection1.setSize(size1D);
    sampleDirection2.setSize(size1D);
    sampleRadiance1.setSize(size1D);
    sampleRadiance2.setSize(size1D);
    sampleWeights1.setSize(size1D);
    sampleWeights2.setSize(size1D);
    motionVectors.setSize(size1D);
}

void VoxelRenderer::handleDirtySizing()
{
    ZoneScoped;

    if (!isSizingDirty)
    {
        return;
    }

    remakeTextures();
}

VoxelRenderer::VoxelRenderer()
{
    prepareRayTraceFromCameraProgram = ShaderManager::getInstance().getComputeProgram(Content::prepareRayTraceFromCameraComputeShader)->programId;
    resetHitInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetHitInfoComputeShader)->programId;
    resetVisualInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetVisualInfoComputeShader)->programId;
    fullCastProgram = ShaderManager::getInstance().getComputeProgram(Content::fullCastComputeShader)->programId;
    afterCastProgram = ShaderManager::getInstance().getComputeProgram(Content::afterCastComputerShader)->programId;

    resetPrimaryRayInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetPrimaryRayInfoComputeShader)->programId;
    beforeCastProgram = ShaderManager::getInstance().getComputeProgram(Content::beforeCastComputeShader)->programId;
    primaryRayProgram = ShaderManager::getInstance().getComputeProgram(Content::castPrimaryRayComputeShader)->programId;
    groupPixelsProgram = ShaderManager::getInstance().getComputeProgram(Content::groupPixelsComputeShader)->programId;

    pathTraceToFramebufferProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::pathTraceToFramebufferShader)->programId;

    glGenBuffers(1, &materialTexturesBuffer); // Generate the buffer that will store the material textures
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

void VoxelRenderer::prepareRayTraceFromCamera(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, bool resetLight)
{
    ZoneScoped;

    handleDirtySizing(); // Handle dirty sizing, this function is supposed to prepare data for rendering, as such it needs to prepare the correct amount of data

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;

    glUseProgram(prepareRayTraceFromCameraProgram);

    if (whichStartBuffer)
    {
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(1);
    }
    else
    {
        rayStartBuffer2.bind(0);
        rayDirectionBuffer2.bind(1);
    }

    // rayPixel.bind(2);
    primaryDirection.bind(2);

    {
        glUniform3i(glGetUniformLocation(prepareRayTraceFromCameraProgram, "resolution"), size.x, size.y, 1);
        glUniform3fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camPosition"), 1, glm::value_ptr(cameraPosition));
        glUniform4fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camRotation"), 1, glm::value_ptr(cameraRotation));
        glUniform1f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "horizontalFovTan"), std::tan(cameraFOV * 0.5));

        glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000000) / 1000000.f, (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }

    if (whichStartBuffer)
    {
        rayStartBuffer1.unbind();
        rayDirectionBuffer1.unbind();
    }
    else
    {
        rayStartBuffer2.unbind();
        rayDirectionBuffer2.unbind();
    }

    primaryDirection.unbind();

    resetPrimaryRayInfo();
    resetVisualInfo(maxDepth);
}

void VoxelRenderer::executeRayTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, int shadingRate, const glm::ivec2& offset, std::shared_ptr<SceneComponent> scene)
{
    ZoneScoped;

    beforeCast(maxDepth, scene, true);

    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(fullCastProgram);

    // bind rayStart info
    if (whichStartBuffer)
    {
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(2);
        rayStartBuffer2.bind(1);
        rayDirectionBuffer2.bind(3);
    }
    else
    {
        rayStartBuffer2.bind(0);
        rayDirectionBuffer2.bind(2);
        rayStartBuffer1.bind(1);
        rayDirectionBuffer1.bind(3);
    }

    // Occupancy Map = 4
    // Material Map = 5

    rayMisc.bind(6);

    MaterialManager& materialManager = MaterialManager::getInstance();
    materialManager.getMaterialDefinitionsBuffer().bind(7); // This binds the material definitions for each material

    if (whichAccumulationBuffer)
    {
        attentuationBuffer1.bind(8);
        accumulatedLightBuffer1.bind(9);
        attentuationBuffer2.bind(10);
        accumulatedLightBuffer2.bind(11);
    }
    else
    {
        attentuationBuffer1.bind(10);
        accumulatedLightBuffer1.bind(11);
        attentuationBuffer2.bind(8);
        accumulatedLightBuffer2.bind(9);
    }

    positionBuffer.bind(12);

    if (whichSampleRadiance)
    {
        sampleDirection1.bind(13);
        sampleRadiance1.bind(14);
        sampleWeights1.bind(15);
    }
    else
    {
        sampleDirection2.bind(13);
        sampleRadiance2.bind(14);
        sampleWeights2.bind(15);
    }

    {
        int divisor = 8 * shadingRate;
        GLuint workGroupsX = (size.x + divisor - 1) / divisor; // Ceiling division
        GLuint workGroupsY = (size.y + divisor - 1) / divisor;

        glUniform3i(glGetUniformLocation(fullCastProgram, "resolution"), size.x, size.y, 1);
        glUniform1f(glGetUniformLocation(fullCastProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation
        glUniform1i(glGetUniformLocation(fullCastProgram, "shadingRate"), shadingRate);
        glUniform2i(glGetUniformLocation(fullCastProgram, "inputOffset"), offset.x, offset.y);

        glUniform1i(glGetUniformLocation(fullCastProgram, "firstMipMapLevel"), firstMipMapLevel);
        glUniform1i(glGetUniformLocation(fullCastProgram, "maxIterations"), maxIterations);

        // The past camera position maps corresponds to the resevoirs
        glUniform3fv(glGetUniformLocation(fullCastProgram, "pastCameraPosition"), 1, glm::value_ptr(pastCameraPosition));
        glUniform4fv(glGetUniformLocation(fullCastProgram, "pastCameraRotation"), 1, glm::value_ptr(pastCameraRotation));
        glUniform1f(glGetUniformLocation(fullCastProgram, "pastCameraFovTan"), std::tan(pastCameraFOV * 0.5)); // A little bit of randomness for temporal accumulation

        std::shared_ptr<SkyboxComponent> skybox = scene->getSkybox();
        glUniform1f(glGetUniformLocation(fullCastProgram, "sunAngularSize"), skybox->getSunAngularSize());
        glUniform3fv(glGetUniformLocation(fullCastProgram, "sunDirection"), 1, glm::value_ptr(skybox->getSunDirection()));

        for (auto& chunkComponent : chunks)
        {
            ZoneScopedN("VoxelRenderer::executeRayTrace - Render chunk preconditions");

            if (!chunkComponent->getRendererData().isVisible)
            {
                continue;
            }

            // Check getExistsOnGpu before locking as an optimization
            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

            std::shared_lock lock(chunkComponent->getMutex());

            // Check again to confirm the chunk data actually exists before using it
            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

            {
                ZoneScopedN("VoxelRenderer::executeRayTrace - Render chunk");

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

                    glDispatchCompute(workGroupsX, workGroupsY, 1);

                    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                }
                chunk->unbindBuffers();
            }
        }
    }

    // unbind rayStart info
    rayStartBuffer1.unbind();
    rayDirectionBuffer1.unbind();
    rayStartBuffer2.unbind();
    rayDirectionBuffer2.unbind();

    rayMisc.unbind();

    materialManager.getMaterialDefinitionsBuffer().unbind();

    attentuationBuffer1.unbind();
    accumulatedLightBuffer1.unbind();
    attentuationBuffer2.unbind();
    accumulatedLightBuffer2.unbind();

    positionBuffer.unbind();

    if (whichSampleRadiance)
    {
        sampleDirection1.unbind();
        sampleRadiance1.unbind();
        sampleWeights1.unbind();
    }
    else
    {
        sampleDirection2.unbind();
        sampleRadiance2.unbind();
        sampleWeights2.unbind();
    }

    glUseProgram(0);

    afterCast(maxDepth);

    whichStartBuffer = !whichStartBuffer;
    whichAccumulationBuffer = !whichAccumulationBuffer;
}

void VoxelRenderer::executePathTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, int bounces, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, std::shared_ptr<SceneComponent> scene)
{
    ZoneScoped;

    executePrimaryRay(chunks, pastCameraPosition, pastCameraRotation, pastCameraFOV, scene);

    glm::ivec2 offset = glm::ivec2(rand(), rand());

    for (int i = 0; i < bounces; i++)
    {
        int shadingRate = i + 1;
        executeRayTrace(chunks, pastCameraPosition, pastCameraRotation, pastCameraFOV, shadingRate, offset, scene);
        // afterCast();
        // resetVisualInfo();
    }
}

void VoxelRenderer::resetPrimaryRayInfo()
{
    ZoneScoped;

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;

    // Reset the hit info
    glUseProgram(resetPrimaryRayInfoProgram);

    normalBuffer.bind(0);
    positionBuffer.bind(1);
    materialUVBuffer.bind(2);

    if (whichStartBuffer)
    {
        rayStartBuffer1.bind(5);
        rayDirectionBuffer1.bind(6);
    }
    else
    {
        rayStartBuffer2.bind(5);
        rayDirectionBuffer2.bind(6);
    }

    glUniform3i(glGetUniformLocation(resetPrimaryRayInfoProgram, "resolution"), size.x, size.y, 1);
    glUniform1f(glGetUniformLocation(resetPrimaryRayInfoProgram, "maxDepth"), maxDepth);

    {
        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    normalBuffer.unbind();
    positionBuffer.unbind();
    materialUVBuffer.unbind();

    if (whichStartBuffer)
    {
        rayStartBuffer1.unbind();
        rayDirectionBuffer1.unbind();
    }
    else
    {
        rayStartBuffer2.unbind();
        rayDirectionBuffer2.unbind();
    }

    glUseProgram(0);
}

void VoxelRenderer::resetVisualInfo(float maxDepth)
{
    ZoneScoped;

    glUseProgram(resetVisualInfoProgram);

    // bind rayStart info
    attentuationBuffer1.bind(0);
    accumulatedLightBuffer1.bind(1);
    attentuationBuffer2.bind(2);
    accumulatedLightBuffer2.bind(3);

    rayMisc.bind(4);

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;

    glUniform3i(glGetUniformLocation(resetVisualInfoProgram, "resolution"), size.x, size.y, 1);
    glUniform1f(glGetUniformLocation(resetVisualInfoProgram, "maxDepth"), maxDepth);

    {
        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    attentuationBuffer1.unbind();
    accumulatedLightBuffer1.unbind();
    attentuationBuffer2.unbind();
    accumulatedLightBuffer2.unbind();

    rayMisc.unbind();

    glUseProgram(0);
}

void VoxelRenderer::beforeCast(float maxDepth, std::shared_ptr<SceneComponent> scene, bool shouldDrawSkybox)
{
    ZoneScoped;

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;

    // Reset the hit info
    glUseProgram(beforeCastProgram);

    rayMisc.bind(0);

    if (whichStartBuffer)
    {
        rayDirectionBuffer1.bind(1);
    }
    else
    {
        rayDirectionBuffer2.bind(1);
    }

    if (whichAccumulationBuffer)
    {
        accumulatedLightBuffer1.bind(2);
        accumulatedLightBuffer2.bind(3);
        attentuationBuffer1.bind(4);
    }
    else
    {
        accumulatedLightBuffer1.bind(3);
        accumulatedLightBuffer2.bind(2);
        attentuationBuffer2.bind(4);
    }

    glUniform3i(glGetUniformLocation(beforeCastProgram, "resolution"), size.x, size.y, 1);
    glUniform1f(glGetUniformLocation(beforeCastProgram, "maxDepth"), maxDepth);
    glUniform1i(glGetUniformLocation(beforeCastProgram, "shouldDrawSkybox"), shouldDrawSkybox);

    std::shared_ptr<SkyboxComponent> skybox = scene->getSkybox();

    glUniform1f(glGetUniformLocation(beforeCastProgram, "sunAngularSize"), skybox->getSunAngularSize());
    if (skybox->getCubemap() != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->getCubemap()->getTextureId());
    }

    glUniform3fv(glGetUniformLocation(beforeCastProgram, "sunDir"), 1, glm::value_ptr(skybox->getSunDirection()));
    glUniform1f(glGetUniformLocation(beforeCastProgram, "sunBrightnessMultiplier"), skybox->getSunBrightnessMultiplier());
    glUniform1f(glGetUniformLocation(beforeCastProgram, "skyBrightnessMultiplier"), skybox->getSkyBrightnessMultiplier());

    {
        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rayMisc.unbind();

    if (whichStartBuffer)
    {
        rayDirectionBuffer1.unbind();
    }
    else
    {
        rayDirectionBuffer2.unbind();
    }

    accumulatedLightBuffer1.unbind();
    accumulatedLightBuffer2.unbind();

    if (whichAccumulationBuffer)
    {
        attentuationBuffer1.unbind();
    }
    else
    {
        attentuationBuffer2.unbind();
    }

    glUseProgram(0);
}

void VoxelRenderer::afterCast(float maxDepth)
{
    ZoneScoped;

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;

    // Reset the hit info
    glUseProgram(afterCastProgram);

    rayMisc.bind(0);

    glUniform3i(glGetUniformLocation(afterCastProgram, "resolution"), size.x, size.y, 1);
    glUniform1f(glGetUniformLocation(afterCastProgram, "maxDepth"), maxDepth);

    {
        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rayMisc.unbind();

    glUseProgram(0);
}

void VoxelRenderer::executePrimaryRay(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, std::shared_ptr<SceneComponent> scene)
{
    ZoneScoped;

    beforeCast(maxDepth, scene, false);

    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(primaryRayProgram);

    // bind rayStart info
    if (whichStartBuffer)
    {
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(2);
        rayStartBuffer2.bind(1);
        rayDirectionBuffer2.bind(3);
    }
    else
    {
        rayStartBuffer2.bind(0);
        rayDirectionBuffer2.bind(2);
        rayStartBuffer1.bind(1);
        rayDirectionBuffer1.bind(3);
    }

    // Occupancy Map = 4
    // Material Map = 5

    MaterialManager& materialManager = MaterialManager::getInstance();
    materialManager.getMaterialDefinitionsBuffer().bind(6); // This binds the material definitions for each material

    rayMisc.bind(7);

    normalBuffer.bind(8);
    positionBuffer.bind(9);
    materialUVBuffer.bind(10);

    materialBuffer.bind(11);
    secondaryDirection.bind(12);
    motionVectors.bind(13);

    std::unordered_set<std::shared_ptr<VoxelChunkComponent>> renderedChunks;
    {
        GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (size.y + 8 - 1) / 8;

        glUniform3i(glGetUniformLocation(primaryRayProgram, "resolution"), size.x, size.y, 1);
        glUniform1f(glGetUniformLocation(primaryRayProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        glUniform3fv(glGetUniformLocation(primaryRayProgram, "pastCameraPosition"), 1, glm::value_ptr(pastCameraPosition));
        glUniform4fv(glGetUniformLocation(primaryRayProgram, "pastCameraRotation"), 1, glm::value_ptr(pastCameraRotation));
        glUniform1f(glGetUniformLocation(primaryRayProgram, "pastCameraFovTan"), std::tan(pastCameraFOV * 0.5)); // A little bit of randomness for temporal accumulation

        std::shared_ptr<SkyboxComponent> skybox = scene->getSkybox();
        glUniform1f(glGetUniformLocation(primaryRayProgram, "sunAngularSize"), skybox->getSunAngularSize());
        glUniform3fv(glGetUniformLocation(primaryRayProgram, "sunDirection"), 1, glm::value_ptr(skybox->getSunDirection()));

        glUniform1i(glGetUniformLocation(primaryRayProgram, "whichAccumulatingVector"), whichMotionVectors);
        glUniform1i(glGetUniformLocation(primaryRayProgram, "whichDepth"), whichDepth);

        glUniform1i(glGetUniformLocation(primaryRayProgram, "firstMipMapLevel"), firstMipMapLevel);
        glUniform1i(glGetUniformLocation(primaryRayProgram, "maxIterations"), maxIterations);

        for (auto& chunkComponent : chunks)
        {
            ZoneScopedN("VoxelRenderer::executePrimaryRay - Render chunk preconditions");

            if (!chunkComponent->getRendererData().isVisible)
            {
                continue;
            }

            // Check getExistsOnGpu before locking as an optimization
            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

            std::shared_lock lock(chunkComponent->getMutex());

            // Check again to confirm the chunk data actually exists before using it
            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

            {
                ZoneScopedN("VoxelRenderer::executePrimaryRay - Render chunk");

                auto& chunk = chunkComponent->getChunk();

                chunk->bindBuffers(4, 5);
                {
                    const auto chunkSize = chunk->getSize();

                    glUniform3i(glGetUniformLocation(primaryRayProgram, "cellCount"), chunkSize.x / 2, chunkSize.y / 2, chunkSize.z / 2);
                    glUniform1ui(glGetUniformLocation(primaryRayProgram, "occupancyMapLayerCount"), chunk->getOccupancyMapIndices().size() - 2);
                    glUniform1uiv(glGetUniformLocation(primaryRayProgram, "occupancyMapIndices"), chunk->getOccupancyMapIndices().size() - 1, chunk->getOccupancyMapIndices().data());

                    glUniform3fv(glGetUniformLocation(primaryRayProgram, "voxelWorldPosition"), 1, glm::value_ptr(chunkComponent->getTransform()->getGlobalPosition()));
                    glUniform4fv(glGetUniformLocation(primaryRayProgram, "voxelWorldRotation"), 1, glm::value_ptr(chunkComponent->getTransform()->getGlobalRotation()));
                    glUniform3fv(glGetUniformLocation(primaryRayProgram, "voxelWorldScale"), 1, glm::value_ptr(chunkComponent->getTransform()->getLossyGlobalScale()));

                    // Load voxel chunk history
                    auto& rendererData = chunkComponent->getRendererData();

                    glUniform3fv(glGetUniformLocation(primaryRayProgram, "pastVoxelWorldPosition"), 1, glm::value_ptr(rendererData.previousPosition));
                    glUniform4fv(glGetUniformLocation(primaryRayProgram, "pastVoxelWorldRotation"), 1, glm::value_ptr(rendererData.previousRotation));
                    glUniform3fv(glGetUniformLocation(primaryRayProgram, "pastVoxelWorldScale"), 1, glm::value_ptr(rendererData.previousScale));

                    glDispatchCompute(workGroupsX, workGroupsY, 1);

                    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                    // Update the history
                    rendererData.previousPosition = chunkComponent->getTransform()->getGlobalPosition();
                    rendererData.previousRotation = chunkComponent->getTransform()->getGlobalRotation();
                    rendererData.previousScale = chunkComponent->getTransform()->getLossyGlobalScale();
                }
                chunk->unbindBuffers();
            }
        }
    }

    // unbind rayStart info
    rayStartBuffer1.unbind();
    rayDirectionBuffer1.unbind();
    rayStartBuffer2.unbind();
    rayDirectionBuffer2.unbind();

    rayMisc.unbind();

    materialManager.getMaterialDefinitionsBuffer().unbind();

    normalBuffer.unbind();
    positionBuffer.unbind();
    materialUVBuffer.unbind();

    materialBuffer.unbind();
    secondaryDirection.unbind();
    motionVectors.unbind();

    afterCast(maxDepth);

    whichStartBuffer = !whichStartBuffer;
    whichMotionVectors = !whichMotionVectors;
    whichDepth = !whichDepth;

    glUseProgram(0);
}

void VoxelRenderer::render(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, std::shared_ptr<SceneComponent> scene)
{
    ZoneScoped;

    // std::cout << "VoxelRenderer display" << std::endl;
    glUseProgram(pathTraceToFramebufferProgram);

    if (whichAccumulationBuffer)
    {
        accumulatedLightBuffer1.bind(0);
    }
    else
    {
        accumulatedLightBuffer2.bind(0);
    }

    normalBuffer.bind(1);
    positionBuffer.bind(2);
    materialUVBuffer.bind(3);

    materialBuffer.bind(4);
    primaryDirection.bind(5);
    secondaryDirection.bind(6);

    MaterialManager& materialManager = MaterialManager::getInstance();
    materialManager.getMaterialDefinitionsBuffer().bind(7); // This binds the material definitions for each material

    if (whichSampleRadiance)
    {
        sampleDirection1.bind(8);
        sampleDirection2.bind(9);

        sampleRadiance1.bind(10);
        sampleRadiance2.bind(11);

        sampleWeights1.bind(12);
        sampleWeights2.bind(13);
    }
    else
    {
        sampleDirection1.bind(9);
        sampleDirection2.bind(8);

        sampleRadiance1.bind(11);
        sampleRadiance2.bind(10);

        sampleWeights1.bind(13);
        sampleWeights2.bind(12);
    }

    motionVectors.bind(14);

    glUniform3i(glGetUniformLocation(pathTraceToFramebufferProgram, "resolution"), size.x, size.y, 1);

    glUniform4fv(glGetUniformLocation(pathTraceToFramebufferProgram, "cameraRotation"), 1, glm::value_ptr(cameraRotation));
    glUniform3fv(glGetUniformLocation(pathTraceToFramebufferProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));

    std::shared_ptr<SkyboxComponent> skybox = scene->getSkybox();
    glUniform1f(glGetUniformLocation(pathTraceToFramebufferProgram, "sunAngularSize"), skybox->getSunAngularSize());
    if (skybox->getCubemap() != nullptr)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->getCubemap()->getTextureId());
    }
    glUniform3fv(glGetUniformLocation(pathTraceToFramebufferProgram, "sunDir"), 1, glm::value_ptr(skybox->getSunDirection()));
    glUniform1f(glGetUniformLocation(pathTraceToFramebufferProgram, "visualMultiplier"), skybox->getVisualMultiplier());

    glUniform1f(glGetUniformLocation(pathTraceToFramebufferProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

    glUniform1i(glGetUniformLocation(pathTraceToFramebufferProgram, "whichMotionVectors"), whichMotionVectors);
    glUniform1i(glGetUniformLocation(pathTraceToFramebufferProgram, "whichDepth"), whichDepth);

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDepthFunc(GL_ALWAYS);
    {
        glDrawBuffers(4, drawBuffers.data());

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    if (whichAccumulationBuffer)
    {
        accumulatedLightBuffer1.unbind();
    }
    else
    {
        accumulatedLightBuffer2.unbind();
    }

    normalBuffer.unbind();
    positionBuffer.unbind();
    materialUVBuffer.unbind();

    materialBuffer.unbind();
    primaryDirection.unbind();
    secondaryDirection.unbind();
    materialManager.getMaterialDefinitionsBuffer().unbind();

    sampleDirection1.unbind();
    sampleDirection2.unbind();
    sampleRadiance1.unbind();
    sampleRadiance2.unbind();
    sampleWeights1.unbind();
    sampleWeights2.unbind();
    motionVectors.unbind();

    whichMotionVectors = !whichMotionVectors;
    whichSampleRadiance = !whichSampleRadiance;

    glUseProgram(0);
}
