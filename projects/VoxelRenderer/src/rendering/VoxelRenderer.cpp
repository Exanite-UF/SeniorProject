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
#include <src/world/VoxelWorld.h>
#include <src/world/MaterialManager.h>
#include <src/world/Material.h>

GLuint VoxelRenderer::prepareRayTraceFromCameraProgram;
GLuint VoxelRenderer::executeRayTraceProgram;
GLuint VoxelRenderer::resetHitInfoProgram;
GLuint VoxelRenderer::displayToWindowProgram;
GLuint VoxelRenderer::BRDFProgram;

void VoxelRenderer::remakeTextures()
{
    isSizingDirty = false;

    // This will delete the texture currently bound to this variable, and set the variable equal to 0
    // If the variable is 0, meaning that no texture is bound, then it will do nothing
    // glDeleteTextures(1, &rayStartBuffer);
    // glDeleteTextures(1, &rayDirectionBuffer);
    uint64_t size = xSize * ySize * raysPerPixel;

    rayHitPositionBuffer.setSize(size);
    rayHitNormalBuffer.setSize(size);
    rayHitMaterialBuffer.setSize(size);
    rayHitVoxelPositionBuffer.setSize(size);

    // Create a new texture
    rayStartBuffer.setSize(size);
    rayDirectionBuffer.setSize(size);

    attentuationBuffer.setSize(size);
    accumulatedLightBuffer.setSize(size);
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
    BRDFProgram = ShaderManager::getManager().getComputeProgram(Content::brdfComputeShader);
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

void VoxelRenderer::prepareRayTraceFromCamera(const Camera& camera)
{
    handleDirtySizing(); // Handle dirty sizing, this function is supposed to prepare data for rendering, as such it needs to prepare the correct amount of data

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
        glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000) / 1000.f, (rand() % 1000) / 1000.f); // A little bit of randomness for temporal accumulation

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
    attentuationBuffer.bind(4);
    accumulatedLightBuffer.bind(5);

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
    attentuationBuffer.unbind();
    accumulatedLightBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::executeRayTrace(std::vector<VoxelWorld>& worlds)
{
    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data

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
                glUniform1ui(glGetUniformLocation(executeRayTraceProgram, "mipMapTextureCount"), voxelWorld.getOccupancyMapIndices().size() - 2);
                glUniform1uiv(glGetUniformLocation(executeRayTraceProgram, "mipMapStartIndices"), voxelWorld.getOccupancyMapIndices().size() - 1, voxelWorld.getOccupancyMapIndices().data());
                glUniform1uiv(glGetUniformLocation(executeRayTraceProgram, "materialStartIndices"), voxelWorld.getMaterialMapIndices().size() - 1, voxelWorld.getMaterialMapIndices().data());

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

void VoxelRenderer::accumulateLight(MaterialManager& materialManager)
{
    // handleDirtySizing();//Do not handle dirty sizing, this function should only be working with data that alreay exist. Resizing would invalidate that data
    glUseProgram(BRDFProgram);

    // These will not be modified
    rayHitPositionBuffer.bind(0);
    rayHitNormalBuffer.bind(1);
    rayHitMaterialBuffer.bind(2);
    rayHitVoxelPositionBuffer.bind(3);

    // These will be modified
    rayStartBuffer.bind(4);
    rayDirectionBuffer.bind(5);
    attentuationBuffer.bind(6);
    accumulatedLightBuffer.bind(7);

    {
        GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (ySize + 8 - 1) / 8;
        GLuint workGroupsZ = raysPerPixel;

        glUniform3i(glGetUniformLocation(BRDFProgram, "resolution"), xSize, ySize, raysPerPixel);
        glUniform1f(glGetUniformLocation(BRDFProgram, "random"), (rand() % 1000) / 1000.f); // A little bit of randomness for temporal accumulation
        
        std::cout << "hi" << std::endl;
        glUniform1ui(glGetUniformLocation(BRDFProgram, "materialMapSize"), Constants::VoxelWorld::materialMapCount);
        glUniform1ui(glGetUniformLocation(BRDFProgram, "materialCount"), Constants::VoxelWorld::materialCount);
        


        // Set the material data
        std::cout << "JHI 1" << std::endl;
        materialManager.getMaterialMapBuffer().bind(8);//This is a mapping from the material index to the material id
        materialManager.getMaterialDataBuffer().bind(9);//This binds the base data for each material
        std::cout << "JHI 2" << std::endl;
        //bind the bindless textures to 10

        // TODO
        //glBindBuffer(GL_UNIFORM_BUFFER, materialTexturesBuffer);
        //glBufferData(GL_UNIFORM_BUFFER, 48 * materialTextures.size() /*Each struct in the buffer must be 48 bytes long*/, materialTextures.data(), GL_DYNAMIC_DRAW /*This can probably be changed to GL_STATIC_DRAW*/); // Actually sets the material data
        //glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // bind light buffer to location 1
        //glBindBufferBase(GL_UNIFORM_BUFFER, glGetUniformLocation(BRDFProgram, "materialTextures"), materialTexturesBuffer);
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); // Shouldn't this be a different barrier


        materialManager.getMaterialMapBuffer().unbind();//This is a mapping from the material index to the material id
        materialManager.getMaterialDataBuffer().unbind();//This binds the base data for each material
    }

    rayHitPositionBuffer.unbind();
    rayHitNormalBuffer.unbind();
    rayHitMaterialBuffer.unbind();
    rayHitVoxelPositionBuffer.unbind();
    rayStartBuffer.unbind();
    rayDirectionBuffer.unbind();
    attentuationBuffer.unbind();
    accumulatedLightBuffer.unbind();

    glUseProgram(0);
}

void VoxelRenderer::display()
{
    glUseProgram(displayToWindowProgram);

    rayHitPositionBuffer.bind(0);
    rayHitNormalBuffer.bind(1);
    rayHitMaterialBuffer.bind(2);
    rayHitVoxelPositionBuffer.bind(3);
    accumulatedLightBuffer.bind(4);

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
    accumulatedLightBuffer.unbind();
    glUseProgram(0);
}
