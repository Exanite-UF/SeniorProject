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
    miscBuffer.setSize(size1D);
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
    prepareRayTraceFromCameraProgram = ShaderManager::getInstance().getComputeProgram(Content::prepareRayTraceFromCameraComputeShader);
    resetHitInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetHitInfoComputeShader);
    resetVisualInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetVisualInfoComputeShader);
    fullCastProgram = ShaderManager::getInstance().getComputeProgram(Content::fullCastComputeShader);
    afterCastProgram = ShaderManager::getInstance().getComputeProgram(Content::afterCastComputerShader);

    resetPrimaryRayInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetPrimaryRayInfoComputeShader);
    beforeCastProgram = ShaderManager::getInstance().getComputeProgram(Content::beforeCastComputeShader);
    primaryRayProgram = ShaderManager::getInstance().getComputeProgram(Content::castPrimaryRayComputeShader);
    groupPixelsProgram = ShaderManager::getInstance().getComputeProgram(Content::groupPixelsComputeShader);

    pathTraceToFramebufferProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::pathTraceToFramebufferShader);

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

    if(whichStartBuffer){
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(1);
    }else{
        rayStartBuffer2.bind(0);
        rayDirectionBuffer2.bind(1);
    }
    
    //rayPixel.bind(2);
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

    if(whichStartBuffer){
        rayStartBuffer1.unbind();
        rayDirectionBuffer1.unbind();
    }else{
        rayStartBuffer2.unbind();
        rayDirectionBuffer2.unbind();
    }

    primaryDirection.unbind();

    

    resetPrimaryRayInfo();
    resetVisualInfo(maxDepth);
}

void VoxelRenderer::executeRayTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, int shadingRate, const glm::ivec2& offset)
{
    ZoneScoped;

    beforeCast(maxDepth);

    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(fullCastProgram);

    // bind rayStart info
    if(whichStartBuffer){
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(2);
        rayStartBuffer2.bind(1);
        rayDirectionBuffer2.bind(3);
    }else{
        rayStartBuffer2.bind(0);
        rayDirectionBuffer2.bind(2);
        rayStartBuffer1.bind(1);
        rayDirectionBuffer1.bind(3);
    }
    

    // Occupancy Map = 5
    // Material Map = 6

    rayMisc.bind(7);

    MaterialManager& materialManager = MaterialManager::getInstance();
    materialManager.getMaterialDefinitionsBuffer().bind(8); // This binds the material definitions for each material

    if(whichAccumulationBuffer){
        attentuationBuffer1.bind(9);
        accumulatedLightBuffer1.bind(10);
        attentuationBuffer2.bind(11);
        accumulatedLightBuffer2.bind(12);
    }else{
        attentuationBuffer1.bind(11);
        accumulatedLightBuffer1.bind(12);
        attentuationBuffer2.bind(9);
        accumulatedLightBuffer2.bind(10);
    }

    {
        int divisor = 8 * shadingRate;
        GLuint workGroupsX = (size.x + divisor - 1) / divisor; // Ceiling division
        GLuint workGroupsY = (size.y + divisor - 1) / divisor;

        glUniform3i(glGetUniformLocation(fullCastProgram, "resolution"), size.x, size.y, 1);
        glUniform1f(glGetUniformLocation(fullCastProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation
        glUniform1i(glGetUniformLocation(fullCastProgram, "shadingRate"), shadingRate);
        glUniform2i(glGetUniformLocation(fullCastProgram, "inputOffset"), offset.x, offset.y);
        
        for (auto& chunkComponent : chunks)
        {
            ZoneScopedN("VoxelRenderer::executeRayTrace - Render chunk");

            std::shared_lock lock(chunkComponent->getMutex());

            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

            auto& chunk = chunkComponent->getChunk();

            chunk->bindBuffers(5, 6);
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

    normalBuffer.unbind();
    positionBuffer.unbind();
    miscBuffer.unbind();

    glUseProgram(0);

    afterCast(maxDepth);

    whichStartBuffer = !whichStartBuffer;
    whichAccumulationBuffer = !whichAccumulationBuffer;
}

void VoxelRenderer::executePathTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, int bounces, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV)
{
    ZoneScoped;

    executePrimaryRay(chunks, pastCameraPosition, pastCameraRotation, pastCameraFOV);
    
    glm::ivec2 offset = glm::ivec2(rand(), rand());

    for (int i = 0; i < bounces; i++)
    {
        int shadingRate = 2;
        if(i > 1){
            shadingRate = 3;
        }
        executeRayTrace(chunks, pastCameraPosition, pastCameraRotation, pastCameraFOV, shadingRate, offset);
        //afterCast();
        //resetVisualInfo();
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
    miscBuffer.bind(2);
    if(whichStartBuffer){
        rayStartBuffer1.bind(5);
        rayDirectionBuffer1.bind(6);
    }else{
        rayStartBuffer2.bind(5);
        rayDirectionBuffer2.bind(6);
    }

    glUniform3i(glGetUniformLocation(resetPrimaryRayInfoProgram, "resolution"), size.x, size.y, 1);

    {
        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    normalBuffer.unbind();
    positionBuffer.unbind();
    miscBuffer.unbind();
    if(whichStartBuffer){
        rayStartBuffer1.unbind();
        rayDirectionBuffer1.unbind();
    }else{
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

void VoxelRenderer::beforeCast(float maxDepth, bool shouldDrawSkybox)
{
    ZoneScoped;

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;

    // Reset the hit info
    glUseProgram(beforeCastProgram);

    rayMisc.bind(0);

    if(whichStartBuffer){
        rayDirectionBuffer1.bind(1);
    }else{
        rayDirectionBuffer2.bind(1);
    }

    if(whichAccumulationBuffer){
        accumulatedLightBuffer1.bind(2);
        accumulatedLightBuffer2.bind(3);
        attentuationBuffer1.bind(4);
    }else{
        accumulatedLightBuffer1.bind(3);
        accumulatedLightBuffer2.bind(2);
        attentuationBuffer2.bind(4);
    }


    glUniform3i(glGetUniformLocation(beforeCastProgram, "resolution"), size.x, size.y, 1);
    glUniform1f(glGetUniformLocation(beforeCastProgram, "maxDepth"), maxDepth);
    glUniform1f(glGetUniformLocation(beforeCastProgram, "sunAngularSize"), sunAngularSize);
    glUniform3f(glGetUniformLocation(beforeCastProgram, "sunDir"), sunDirection.x, sunDirection.y, sunDirection.z);
    glUniform1f(glGetUniformLocation(beforeCastProgram, "sunBrightness"), sunBrightness);
    glUniform3f(glGetUniformLocation(beforeCastProgram, "skyColor"), skyColor.x, skyColor.y, skyColor.z);
    glUniform3f(glGetUniformLocation(beforeCastProgram, "groundColor"), groundColor.x, groundColor.y, groundColor.z);
    glUniform1i(glGetUniformLocation(beforeCastProgram, "shouldDrawSkybox"), shouldDrawSkybox);

    {
        glDispatchCompute(workGroupsX, workGroupsY, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rayMisc.unbind();

    if(whichStartBuffer){
        rayDirectionBuffer1.unbind();
    }else{
        rayDirectionBuffer2.unbind();
    }

    accumulatedLightBuffer1.unbind();
    accumulatedLightBuffer2.unbind();

    if(whichAccumulationBuffer){
        attentuationBuffer1.unbind();
    }else{
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

void VoxelRenderer::executePrimaryRay(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV)
{
    ZoneScoped;

    beforeCast(maxDepth, false);

    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(primaryRayProgram);

    // bind rayStart info
    if(whichStartBuffer){
        rayStartBuffer1.bind(0);
        rayDirectionBuffer1.bind(2);
        rayStartBuffer2.bind(1);
        rayDirectionBuffer2.bind(3);
    }else{
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
    miscBuffer.bind(10);

    materialBuffer.bind(11);
    secondaryDirection.bind(12);

    std::unordered_set<std::shared_ptr<VoxelChunkComponent>> renderedChunks;
    {
        GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (size.y + 8 - 1) / 8;

        glUniform3i(glGetUniformLocation(primaryRayProgram, "resolution"), size.x, size.y, 1);
        glUniform1f(glGetUniformLocation(primaryRayProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        glUniform3fv(glGetUniformLocation(primaryRayProgram, "pastCameraPosition"), 1, glm::value_ptr(pastCameraPosition));
        glUniform4fv(glGetUniformLocation(primaryRayProgram, "pastCameraRotation"), 1, glm::value_ptr(pastCameraRotation));
        glUniform1f(glGetUniformLocation(primaryRayProgram, "pastCameraFovTan"), std::tan(pastCameraFOV * 0.5)); // A little bit of randomness for temporal accumulation

        glUniform1f(glGetUniformLocation(primaryRayProgram, "sunAngularSize"), sunAngularSize);
        glUniform3f(glGetUniformLocation(primaryRayProgram, "sunDirection"), sunDirection.x, sunDirection.y, sunDirection.z);

        for (auto& chunkComponent : chunks)
        {
            ZoneScopedN("VoxelRenderer::executeRayTrace - Render chunk");

            std::shared_lock lock(chunkComponent->getMutex());

            if (!chunkComponent->getExistsOnGpu())
            {
                continue;
            }

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
                if (voxelChunkHistories.count(chunkComponent) == 0)
                {
                    // Then no history exists
                    // voxelChunkHistories.insert(std::make_pair(chunkComponent, ));
                    voxelChunkHistories.emplace(chunkComponent, VoxelChunkHistory(chunkComponent->getTransform()->getGlobalPosition(), chunkComponent->getTransform()->getGlobalRotation(), chunkComponent->getTransform()->getLossyGlobalScale()));
                }

                VoxelChunkHistory history = voxelChunkHistories.at(chunkComponent);

                glUniform3fv(glGetUniformLocation(primaryRayProgram, "pastVoxelWorldPosition"), 1, glm::value_ptr(history.position));
                glUniform4fv(glGetUniformLocation(primaryRayProgram, "pastVoxelWorldRotation"), 1, glm::value_ptr(history.rotation));
                glUniform3fv(glGetUniformLocation(primaryRayProgram, "pastVoxelWorldScale"), 1, glm::value_ptr(history.scale));

                glDispatchCompute(workGroupsX, workGroupsY, 1);

                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // Update the history
                voxelChunkHistories.at(chunkComponent) = VoxelChunkHistory(chunkComponent->getTransform()->getGlobalPosition(), chunkComponent->getTransform()->getGlobalRotation(), chunkComponent->getTransform()->getLossyGlobalScale());
                renderedChunks.insert(chunkComponent);
            }
            chunk->unbindBuffers();
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
    miscBuffer.unbind();

    materialBuffer.unbind();
    secondaryDirection.unbind();

    afterCast(maxDepth);

    whichStartBuffer = !whichStartBuffer;

    glUseProgram(0);

    
}

void VoxelRenderer::render(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV)
{
    ZoneScoped;

    // std::cout << "VoxelRenderer display" << std::endl;
    glUseProgram(pathTraceToFramebufferProgram);

    if(whichAccumulationBuffer){
        accumulatedLightBuffer1.bind(0);
    }else{
        accumulatedLightBuffer2.bind(0);
    }
    

    normalBuffer.bind(1);
    positionBuffer.bind(2);
    miscBuffer.bind(3);

    materialBuffer.bind(4);
    primaryDirection.bind(5);
    secondaryDirection.bind(6);

    MaterialManager& materialManager = MaterialManager::getInstance();
    materialManager.getMaterialDefinitionsBuffer().bind(7); // This binds the material definitions for each material


    glUniform3i(glGetUniformLocation(pathTraceToFramebufferProgram, "resolution"), size.x, size.y, 1);

    glUniform4fv(glGetUniformLocation(pathTraceToFramebufferProgram, "cameraRotation"), 1, glm::value_ptr(cameraRotation));
    glUniform3fv(glGetUniformLocation(pathTraceToFramebufferProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
    glUniform1f(glGetUniformLocation(pathTraceToFramebufferProgram, "sunAngularSize"), sunAngularSize);
    glUniform3f(glGetUniformLocation(pathTraceToFramebufferProgram, "sunDir"), sunDirection.x, sunDirection.y, sunDirection.z);
    glUniform1f(glGetUniformLocation(pathTraceToFramebufferProgram, "sunBrightness"), sunBrightness);
    glUniform3f(glGetUniformLocation(pathTraceToFramebufferProgram, "skyColor"), skyColor.x, skyColor.y, skyColor.z);
    glUniform3f(glGetUniformLocation(pathTraceToFramebufferProgram, "groundColor"), groundColor.x, groundColor.y, groundColor.z);

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDepthFunc(GL_ALWAYS);
    {
        glDrawBuffers(4, drawBuffers.data());

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(whichAccumulationBuffer){
        accumulatedLightBuffer1.unbind();
    }else{
        accumulatedLightBuffer2.unbind();
    }

    normalBuffer.unbind();
    positionBuffer.unbind();
    miscBuffer.unbind();

    materialBuffer.unbind();
    primaryDirection.unbind();
    secondaryDirection.unbind();
    materialManager.getMaterialDefinitionsBuffer().unbind();

    glUseProgram(0);
}
