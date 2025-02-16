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
    glDeleteTextures(1, &rayHitPositionBuffer);
    glDeleteTextures(1, &rayHitNormalBuffer);
    glDeleteTextures(1, &rayHitMaterialBuffer);

    // Create a new texture
    rayStartBuffer.resize(xSize * ySize * raysPerPixel * 3);
    rayDirectionBuffer.resize(xSize * ySize * raysPerPixel * 3);

    // rayStartBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    // rayDirectionBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    rayHitPositionBuffer = GraphicsUtility::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayHitNormalBuffer = GraphicsUtility::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayHitMaterialBuffer = GraphicsUtility::create3DImage(xSize, ySize, raysPerPixel, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT);
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
    handleDirtySizing();

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
        glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000) / 1000.f, (rand() % 1000) / 1000.f);

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }
    rayStartBuffer.unbind();
    rayDirectionBuffer.unbind();

    // Reset the hit info
    glUseProgram(resetHitInfoProgram);

    glBindImageTexture(
        0, // Image unit index (matches binding=0)
        rayHitPositionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=0)
        rayHitNormalBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=0)
        rayHitMaterialBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R16UI // Format
    );
    {
        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    glBindImageTexture(
        0, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_WRITE_ONLY, // Access qualifier
        GL_R16UI // Format
    );

    glUseProgram(0);
}

void VoxelRenderer::executeRayTrace(std::vector<VoxelWorld>& worlds)
{
    handleDirtySizing();

    glUseProgram(executeRayTraceProgram);

    // bind rayStart info
    rayStartBuffer.bind(0);
    rayDirectionBuffer.bind(1);

    // bind hit info
    glBindImageTexture(
        5, // Image unit index (matches binding=0)
        rayHitPositionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        6, // Image unit index (matches binding=0)
        rayHitNormalBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        7, // Image unit index (matches binding=0)
        rayHitMaterialBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_R16UI // Format
    );

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
                glUniform1ui(glGetUniformLocation(executeRayTraceProgram, "mipMapTextureCount"), voxelWorld.getOccupancyStartIndices().size() - 1);
                glUniform1uiv(glGetUniformLocation(executeRayTraceProgram, "mipMapStartIndices"), voxelWorld.getOccupancyStartIndices().size(), voxelWorld.getOccupancyStartIndices().data());
                glUniform1uiv(glGetUniformLocation(executeRayTraceProgram, "materialStartIndices"), voxelWorld.getMaterialStartIndices().size(), voxelWorld.getMaterialStartIndices().data());

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

    // unbind hit info
    glBindImageTexture(
        5, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        6, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        7, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_R16UI // Format
    );

    glUseProgram(0);
}

void VoxelRenderer::display()
{
    glUseProgram(displayToWindowProgram);

    glBindImageTexture(
        0, // Image unit index (matches binding=0)
        rayHitPositionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=0)
        rayHitNormalBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=0)
        rayHitMaterialBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_R16UI // Format
    );

    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
    {
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glBindVertexArray(0);

    glBindImageTexture(
        0, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_R16UI // Format
    );

    glUseProgram(0);
}
