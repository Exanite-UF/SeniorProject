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

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/TupleHasher.h>
#include <src/world/VoxelWorld.h>
#include "VoxelRenderer.h"

GLuint VoxelRenderer::prepareRayTraceFromCameraProgram;
GLuint VoxelRenderer::executeRayTraceProgram;
GLuint VoxelRenderer::resetHitInfoProgram;
GLuint VoxelRenderer::displayToWindowProgram;

void VoxelRenderer::remakeTextures()
{
    isSizingDirty = false;

    // This will delete the texture currently bound to this variable, and set the variable equal to 0
    // If the variable is 0, meaning that no texture is bound, then it will do nothing
    // glDeleteTextures(1, &rayStartBuffer);
    // glDeleteTextures(1, &rayDirectionBuffer);
    uint64_t size = xSize * ySize * raysPerPixel;

    rayHitPositionBuffer.resize(size);
    rayHitNormalBuffer.resize(size);
    rayHitMaterialBuffer.resize(size);
    rayHitVoxelPositionBuffer.resize(size);

    // Create a new texture
    rayStartBuffer.resize(size);
    rayDirectionBuffer.resize(size);
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
    prepareRayTraceFromCameraProgram = ShaderManager::getManager().getComputeProgram(Content::prepareRayTraceFromCameraComputeShader);
    executeRayTraceProgram = ShaderManager::getManager().getComputeProgram(Content::executeRayTraceComputeShader);
    resetHitInfoProgram = ShaderManager::getManager().getComputeProgram(Content::resetHitInfoComputeShader);
    displayToWindowProgram = ShaderManager::getManager().getGraphicsProgram(Content::screenTriVertexShader, Content::displayToWindowFragmentShader);
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

void VoxelRenderer::prepareRayTraceFromCamera(const Camera& camera)
{
    handleDirtySizing();//Handle dirty sizing, this function is supposed to prepare data for rendering, as such it needs to prepare the correct amount of data

    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glUseProgram(prepareRayTraceFromCameraProgram);
    rayStartBuffer.bind(0);
    rayDirectionBuffer.bind(1);
    {
        glUniform3i(glGetUniformLocation(prepareRayTraceFromCameraProgram, "resolution"), xSize, ySize, raysPerPixel);
        glUniform3fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camPosition"), 1, glm::value_ptr(camera.transform.getGlobalPosition()));
        glUniform4fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camRotation"), 1, glm::value_ptr(camera.transform.getGlobalRotation()));
        glUniform1f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "horizontalFovTan"), camera.getHorizontalFov());
        glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000) / 1000.f, (rand() % 1000) / 1000.f);//A little bit of randomness for temporal accumulation

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }
    rayStartBuffer.unbind();
    rayDirectionBuffer.unbind();

    // Reset the hit info
    glUseProgram(resetHitInfoProgram);

    rayHitPositionBuffer.bind(0);
    rayHitNormalBuffer.bind(1);
    rayHitMaterialBuffer.bind(2);
    rayHitVoxelPositionBuffer.bind(3);

    glUniform3i(glGetUniformLocation(resetHitInfoProgram, "resolution"), xSize, ySize, raysPerPixel);

    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    rayHitPositionBuffer.unbind();
    rayHitNormalBuffer.unbind();
    rayHitMaterialBuffer.unbind();
    rayHitVoxelPositionBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::executeRayTrace(std::vector<VoxelWorld>& worlds)
{
    //handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data

    glUseProgram(executeRayTraceProgram);

    // bind rayStart info
    rayStartBuffer.bind(0);
    rayDirectionBuffer.bind(1);

    rayHitPositionBuffer.bind(4);
    rayHitNormalBuffer.bind(5);
    rayHitMaterialBuffer.bind(6);
    rayHitVoxelPositionBuffer.bind(7);

    {
        GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (ySize + 8 - 1) / 8;
        GLuint workGroupsZ = raysPerPixel;

        glUniform3i(glGetUniformLocation(executeRayTraceProgram, "resolution"), xSize, ySize, raysPerPixel);

        for (auto& voxelWorld : worlds)
        {
            voxelWorld.bindBuffers(2, 3);
            {
                glm::ivec3 voxelSize = voxelWorld.getSize();
                // std::cout << voxelSize.x / 2 << " " << voxelSize.y / 2 << " " << voxelSize.z / 2 << std::endl;
                // std::cout << voxelWorld.getMipMapStarts().size() << std::endl;

                glUniform3i(glGetUniformLocation(executeRayTraceProgram, "voxelResolution"), voxelSize.x / 2, voxelSize.y / 2, voxelSize.z / 2);
                glUniform1ui(glGetUniformLocation(executeRayTraceProgram, "mipMapTextureCount"), voxelWorld.getMipMapTextureCount());
                glUniform1uiv(glGetUniformLocation(executeRayTraceProgram, "mipMapStartIndices"), 10, voxelWorld.getMipMapStartIndices().data());
                glUniform1uiv(glGetUniformLocation(executeRayTraceProgram, "materialStartIndices"), 3, voxelWorld.getMaterialStartIndices().data());

                glUniform3fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldPosition"), 1, glm::value_ptr(voxelWorld.transform.getGlobalPosition()));
                glUniform4fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldRotation"), 1, glm::value_ptr(voxelWorld.transform.getGlobalRotation()));
                glUniform3fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldScale"), 1, glm::value_ptr(voxelWorld.transform.getGlobalScale()));

                glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
            voxelWorld.unbindBuffers();
        }
    }

    // unbind rayStart info
    rayStartBuffer.unbind();
    rayDirectionBuffer.unbind();

    rayHitPositionBuffer.unbind();
    rayHitNormalBuffer.unbind();
    rayHitMaterialBuffer.unbind();
    rayHitVoxelPositionBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::accumulateLight(const std::array<uint32_t, 4096>& materialMap, const std::array<float, 1024>& materialTextureSizes)
{
    //handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(BRDFProgram);

    //These will not be modified
    rayHitPositionBuffer.bind(0);
    rayHitNormalBuffer.bind(1);
    rayHitMaterialBuffer.bind(2);
    rayHitVoxelPositionBuffer.bind(3);

    //These will be modified
    rayStartBuffer.bind(4);
    rayDirectionBuffer.bind(5);

    attentuationBuffer.bind(6);
    accumulatedLightBuffer.bind(7);
    //6 is the prior attenuation
    //7 is the accumulated light
    {
        glUniform3i(glGetUniformLocation(BRDFProgram, "resolution"), xSize, ySize, raysPerPixel);
        glUniform1f(glGetUniformLocation(BRDFProgram, "random"), (rand() % 1000) / 1000.f);//A little bit of randomness for temporal accumulation
        glUniform1uiv(glGetUniformLocation(BRDFProgram, "materialMap"), 4096, materialMap.data());
        glUniform2fv(glGetUniformLocation(BRDFProgram, "sizes"), 512, materialTextureSizes.data());



    }
    



    glUseProgram(0);
}

void VoxelRenderer::display()
{
    glUseProgram(displayToWindowProgram);


    rayHitPositionBuffer.bind(0);
    rayHitNormalBuffer.bind(1);
    rayHitMaterialBuffer.bind(2);
    rayHitVoxelPositionBuffer.bind(3);

    glUniform3i(glGetUniformLocation(displayToWindowProgram, "resolution"), xSize, ySize, raysPerPixel);

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);

    rayHitPositionBuffer.unbind();
    rayHitNormalBuffer.unbind();
    rayHitMaterialBuffer.unbind();
    rayHitVoxelPositionBuffer.unbind();

    glUseProgram(0);
}
