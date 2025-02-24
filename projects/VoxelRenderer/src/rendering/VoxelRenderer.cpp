#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>

#include "VoxelRenderer.h"
#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/TupleHasher.h>
#include <src/world/Material.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelWorld.h>

GLuint VoxelRenderer::prepareRayTraceFromCameraProgram;
GLuint VoxelRenderer::executeRayTraceProgram;
GLuint VoxelRenderer::resetHitInfoProgram;
GLuint VoxelRenderer::displayToWindowProgram;
GLuint VoxelRenderer::BRDFProgram;
GLuint VoxelRenderer::resetVisualInfoProgram;
GLuint VoxelRenderer::fullCastProgram;

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

    // Create a new texture
    rayStartBuffer1.setSize(size1D);
    rayDirectionBuffer1.setSize(size1D);
    rayStartBuffer2.setSize(size1D);
    rayDirectionBuffer2.setSize(size1D);

    accumulatedLightBuffer.setSize(size1D);
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
    executeRayTraceProgram = ShaderManager::getInstance().getComputeProgram(Content::executeRayTraceComputeShader);
    resetHitInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetHitInfoComputeShader);
    displayToWindowProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::displayToWindowFragmentShader);
    BRDFProgram = ShaderManager::getInstance().getComputeProgram(Content::brdfComputeShader);
    resetVisualInfoProgram = ShaderManager::getInstance().getComputeProgram(Content::resetVisualInfoComputeShader);
    fullCastProgram = ShaderManager::getInstance().getComputeProgram(Content::fullCastComputeShader);
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

void VoxelRenderer::setRaysPerPixel(int number)
{
    if (raysPerPixel == number)
    {
        return;
    }

    raysPerPixel = number;
    isSizingDirty = true;
}

void VoxelRenderer::prepareRayTraceFromCamera(const Camera& camera, bool resetLight)
{
    handleDirtySizing(); // Handle dirty sizing, this function is supposed to prepare data for rendering, as such it needs to prepare the correct amount of data

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glUseProgram(prepareRayTraceFromCameraProgram);

    rayStartBuffer1.bind(0);
    rayDirectionBuffer1.bind(1);

    // attentuationBuffer1.bind(2);
    // accumulatedLightBuffer1.bind(3);
    // attentuationBuffer2.bind(4);
    // accumulatedLightBuffer2.bind(5);

    {
        glUniform3i(glGetUniformLocation(prepareRayTraceFromCameraProgram, "resolution"), size.x, size.y, raysPerPixel);
        glUniform3fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camPosition"), 1, glm::value_ptr(camera.transform.getGlobalPosition()));

        glUniform4fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camRotation"), 1, glm::value_ptr(camera.transform.getGlobalRotation()));
        glUniform1f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "horizontalFovTan"), camera.getHorizontalFov());
        glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000000) / 1000000.f, (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }

    rayStartBuffer1.unbind();
    rayDirectionBuffer1.unbind();

    // attentuationBuffer1.unbind();
    // accumulatedLightBuffer1.unbind();
    // attentuationBuffer2.unbind();
    // accumulatedLightBuffer2.unbind();

    resetHitInfo();

    resetVisualInfo(resetLight);
    isFirstRay = true;
}

void VoxelRenderer::executeRayTrace(std::array<std::shared_ptr<VoxelWorld>, 8>& worlds, MaterialManager& materialManager, int bounces)
{
    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    
    glUseProgram(fullCastProgram);
    outputBuffer = bounces + 1;//Used so that display will display from the correct buffer

    // bind rayStart info
    rayStartBuffer1.bind(0);
    rayDirectionBuffer1.bind(1);
    rayStartBuffer2.bind(2);
    rayDirectionBuffer2.bind(3);

    // Occupancy Map = 4
    // Material Map = 5

    rayHitMiscBuffer.bind(4);

    materialManager.getMaterialMapBuffer().bind(5); // This is a mapping from the material index to the material id
    materialManager.getMaterialDataBuffer().bind(6); // This binds the base data for each material

    accumulatedLightBuffer.bind(10); // Input

    normalBuffer.bind(11);
    positionBuffer.bind(12);

    {
        GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (size.y + 8 - 1) / 8;
        GLuint workGroupsZ = raysPerPixel;

        glUniform3i(glGetUniformLocation(fullCastProgram, "resolution"), size.x, size.y, raysPerPixel);

        glUniform1i(glGetUniformLocation(fullCastProgram, "bounces"), bounces);

        glUniform1ui(glGetUniformLocation(fullCastProgram, "materialMapSize"), Constants::VoxelWorld::materialMapCount);
        glUniform1ui(glGetUniformLocation(fullCastProgram, "materialCount"), Constants::VoxelWorld::materialCount);
        glUniform1f(glGetUniformLocation(fullCastProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        for (int i = 0; i < 1; i++)
        {
            std::shared_ptr<VoxelWorld>& voxelWorld = worlds[i];
            
            std::string temp = "isVoxelWorldLoaded" + std::to_string(i + 1);
            if(voxelWorld == nullptr){
                
                glUniform1i(glGetUniformLocation(fullCastProgram, temp.c_str()), false);
                continue;
            }

            voxelWorld->bindBuffers(13 + i, 21 + i);


            glm::ivec3 voxelSize = voxelWorld->getSize();
            // std::cout << voxelSize.x / 2 << " " << voxelSize.y / 2 << " " << voxelSize.z / 2 << std::endl;
            // std::cout << voxelWorld.getMipMapStarts().size() << std::endl;
            
            glUniform1i(glGetUniformLocation(fullCastProgram, temp.c_str()), true);

            temp = "voxelResolution" + std::to_string(i + 1);
            glUniform3i(glGetUniformLocation(fullCastProgram, temp.c_str()), voxelSize.x / 2, voxelSize.y / 2, voxelSize.z / 2);
            temp = "mipMapTextureCount" + std::to_string(i + 1);
            glUniform1ui(glGetUniformLocation(fullCastProgram, temp.c_str()), voxelWorld->getOccupancyMapIndices().size() - 2);
            temp = "mipMapStartIndices" + std::to_string(i + 1);
            glUniform1uiv(glGetUniformLocation(fullCastProgram, temp.c_str()), voxelWorld->getOccupancyMapIndices().size() - 1, voxelWorld->getOccupancyMapIndices().data());
            temp = "materialStartIndices" + std::to_string(i + 1);
            glUniform1uiv(glGetUniformLocation(fullCastProgram, temp.c_str()), voxelWorld->getMaterialMapIndices().size() - 1, voxelWorld->getMaterialMapIndices().data());

            temp = "voxelWorldPosition" + std::to_string(i + 1);
            glUniform3fv(glGetUniformLocation(fullCastProgram, temp.c_str()), 1, glm::value_ptr(voxelWorld->transform.getGlobalPosition()));
            temp = "voxelWorldRotation" + std::to_string(i + 1);
            glUniform4fv(glGetUniformLocation(fullCastProgram, temp.c_str()), 1, glm::value_ptr(voxelWorld->transform.getGlobalRotation()));
            temp = "voxelWorldScale" + std::to_string(i + 1);
            glUniform3fv(glGetUniformLocation(fullCastProgram, temp.c_str()), 1, glm::value_ptr(voxelWorld->transform.getGlobalScale()));
        }

        {
            glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        for(auto& voxelWorld : worlds){
            if(voxelWorld == nullptr){
                continue;
            }
            voxelWorld->unbindBuffers();
        }
    }

    // unbind rayStart info
    rayStartBuffer1.unbind();
    rayDirectionBuffer1.unbind();
    rayStartBuffer2.unbind();
    rayDirectionBuffer2.unbind();

    rayHitMiscBuffer.unbind();

    materialManager.getMaterialMapBuffer().unbind();
    materialManager.getMaterialDataBuffer().unbind();

    accumulatedLightBuffer.unbind();


    normalBuffer.unbind();
    positionBuffer.unbind();

    glUseProgram(0);

    isFirstRay = false;
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

void VoxelRenderer::resetVisualInfo(bool resetLight)
{
    glUseProgram(resetVisualInfoProgram);

    accumulatedLightBuffer.bind(0);
    normalBuffer.bind(4);
    positionBuffer.bind(5);

    GLuint workGroupsX = (size.x + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glUniform3i(glGetUniformLocation(resetVisualInfoProgram, "resolution"), size.x, size.y, raysPerPixel);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "resetLight"), resetLight);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    accumulatedLightBuffer.unbind();
    normalBuffer.unbind();
    positionBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::display(const Camera& camera, int frameCount)
{
    glUseProgram(displayToWindowProgram);

    accumulatedLightBuffer.bind(0);

    normalBuffer.bind(1);
    positionBuffer.bind(2);

    glUniform3i(glGetUniformLocation(displayToWindowProgram, "resolution"), size.x, size.y, raysPerPixel);
    glUniform1i(glGetUniformLocation(displayToWindowProgram, "frameCount"), frameCount);

    glUniform4fv(glGetUniformLocation(displayToWindowProgram, "cameraRotation"), 1, glm::value_ptr(camera.transform.getGlobalRotation()));
    glUniform3fv(glGetUniformLocation(displayToWindowProgram, "cameraPosition"), 1, glm::value_ptr(camera.transform.getGlobalPosition()));

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);

    accumulatedLightBuffer.unbind();

    normalBuffer.unbind();
    positionBuffer.unbind();

    glUseProgram(0);
}
