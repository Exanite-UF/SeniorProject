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
    uint64_t size = xSize * ySize * raysPerPixel;

    rayHitMiscBuffer.setSize(2 * size);

    normalBuffer.setSize(xSize * ySize);
    positionBuffer.setSize(xSize * ySize);

    // Create a new texture
    rayStartBuffer1.setSize(size);
    rayDirectionBuffer1.setSize(size);
    rayStartBuffer2.setSize(size);
    rayDirectionBuffer2.setSize(size);

    attentuationBuffer1.setSize(size);
    accumulatedLightBuffer1.setSize(size);
    attentuationBuffer2.setSize(size);
    accumulatedLightBuffer2.setSize(size);
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

void VoxelRenderer::setResolution(int x, int y)
{
    if (xSize == x && ySize == y)
    {
        return;
    }

    xSize = x;
    ySize = y;
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

    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
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

    // attentuationBuffer1.bind(2);
    // accumulatedLightBuffer1.bind(3);
    // attentuationBuffer2.bind(4);
    // accumulatedLightBuffer2.bind(5);

    {
        glUniform3i(glGetUniformLocation(prepareRayTraceFromCameraProgram, "resolution"), xSize, ySize, raysPerPixel);
        glUniform3fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camPosition"), 1, glm::value_ptr(camera.transform.getGlobalPosition()));

        glUniform4fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camRotation"), 1, glm::value_ptr(camera.transform.getGlobalRotation()));
        glUniform1f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "horizontalFovTan"), camera.getHorizontalFov());
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

    // attentuationBuffer1.unbind();
    // accumulatedLightBuffer1.unbind();
    // attentuationBuffer2.unbind();
    // accumulatedLightBuffer2.unbind();

    resetHitInfo();

    resetVisualInfo(resetLight);
    isFirstRay = true;
}

void VoxelRenderer::executeRayTrace(std::vector<VoxelWorld>& worlds, MaterialManager& materialManager)
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

    materialManager.getMaterialMapBuffer().bind(7); // This is a mapping from the material index to the material id
    materialManager.getMaterialDataBuffer().bind(8); // This binds the base data for each material

    if (currentBuffer % 2 == 0)
    {
        attentuationBuffer1.bind(9); // Input
        accumulatedLightBuffer1.bind(10); // Input
        attentuationBuffer2.bind(11); // Output
        accumulatedLightBuffer2.bind(12); // Output
    }
    else
    {
        attentuationBuffer1.bind(11); // Output
        accumulatedLightBuffer1.bind(12); // Output
        attentuationBuffer2.bind(9); // Input
        accumulatedLightBuffer2.bind(10); // Input
    }

    normalBuffer.bind(13);
    positionBuffer.bind(14);

    {
        GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (ySize + 8 - 1) / 8;
        GLuint workGroupsZ = raysPerPixel;

        glUniform3i(glGetUniformLocation(fullCastProgram, "resolution"), xSize, ySize, raysPerPixel);

        glUniform1i(glGetUniformLocation(fullCastProgram, "isFirstRay"), isFirstRay);

        glUniform1ui(glGetUniformLocation(fullCastProgram, "materialMapSize"), Constants::VoxelWorld::materialMapCount);
        glUniform1ui(glGetUniformLocation(fullCastProgram, "materialCount"), Constants::VoxelWorld::materialCount);
        glUniform1f(glGetUniformLocation(fullCastProgram, "random"), (rand() % 1000000) / 1000000.f); // A little bit of randomness for temporal accumulation

        for (auto& voxelWorld : worlds)
        {
            voxelWorld.bindBuffers(4, 5);
            {
                glm::ivec3 voxelSize = voxelWorld.getSize();
                // std::cout << voxelSize.x / 2 << " " << voxelSize.y / 2 << " " << voxelSize.z / 2 << std::endl;
                // std::cout << voxelWorld.getMipMapStarts().size() << std::endl;

                glUniform3i(glGetUniformLocation(fullCastProgram, "voxelResolution"), voxelSize.x / 2, voxelSize.y / 2, voxelSize.z / 2);
                glUniform1ui(glGetUniformLocation(fullCastProgram, "mipMapTextureCount"), voxelWorld.getOccupancyMapIndices().size() - 2);
                glUniform1uiv(glGetUniformLocation(fullCastProgram, "mipMapStartIndices"), voxelWorld.getOccupancyMapIndices().size() - 1, voxelWorld.getOccupancyMapIndices().data());
                glUniform1uiv(glGetUniformLocation(fullCastProgram, "materialStartIndices"), voxelWorld.getMaterialMapIndices().size() - 1, voxelWorld.getMaterialMapIndices().data());

                glUniform3fv(glGetUniformLocation(fullCastProgram, "voxelWorldPosition"), 1, glm::value_ptr(voxelWorld.transform.getGlobalPosition()));
                glUniform4fv(glGetUniformLocation(fullCastProgram, "voxelWorldRotation"), 1, glm::value_ptr(voxelWorld.transform.getGlobalRotation()));
                glUniform3fv(glGetUniformLocation(fullCastProgram, "voxelWorldScale"), 1, glm::value_ptr(voxelWorld.transform.getGlobalScale()));

                glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            }
            voxelWorld.unbindBuffers();
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

    attentuationBuffer1.unbind();
    accumulatedLightBuffer1.unbind();
    attentuationBuffer2.unbind();
    accumulatedLightBuffer2.unbind();

    normalBuffer.unbind();
    positionBuffer.unbind();

    glUseProgram(0);

    currentBuffer++;

    isFirstRay = false;
}

void VoxelRenderer::resetHitInfo()
{
    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    // Reset the hit info
    glUseProgram(resetHitInfoProgram);

    rayHitMiscBuffer.bind(0);

    glUniform3i(glGetUniformLocation(resetHitInfoProgram, "resolution"), xSize, ySize, raysPerPixel);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    rayHitMiscBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::resetVisualInfo(bool resetLight, bool resetAttentuation)
{
    glUseProgram(resetVisualInfoProgram);

    attentuationBuffer1.bind(0);
    accumulatedLightBuffer1.bind(1);
    attentuationBuffer2.bind(2);
    accumulatedLightBuffer2.bind(3);
    normalBuffer.bind(4);
    positionBuffer.bind(5);

    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glUniform3i(glGetUniformLocation(resetVisualInfoProgram, "resolution"), xSize, ySize, raysPerPixel);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "resetLight"), resetLight);
    glUniform1i(glGetUniformLocation(resetVisualInfoProgram, "resetAttentuation"), resetAttentuation);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    attentuationBuffer1.unbind();
    accumulatedLightBuffer1.unbind();
    attentuationBuffer2.unbind();
    accumulatedLightBuffer2.unbind();
    normalBuffer.unbind();
    positionBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::display(const Camera& camera, int frameCount)
{
    glUseProgram(displayToWindowProgram);

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

    glUniform3i(glGetUniformLocation(displayToWindowProgram, "resolution"), xSize, ySize, raysPerPixel);
    glUniform1i(glGetUniformLocation(displayToWindowProgram, "frameCount"), frameCount);

    glUniform4fv(glGetUniformLocation(displayToWindowProgram, "cameraRotation"), 1, glm::value_ptr(camera.transform.getGlobalRotation()));
    glUniform3fv(glGetUniformLocation(displayToWindowProgram, "cameraPosition"), 1, glm::value_ptr(camera.transform.getGlobalPosition()));

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);

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

    glUseProgram(0);
}
