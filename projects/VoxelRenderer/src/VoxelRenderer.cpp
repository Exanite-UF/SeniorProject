#include "VoxelRenderer.h"
#include "GraphicsUtils.h"
#include "ShaderManager.h"
#include "TupleHasher.h"
#include "VoxelWorld.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <string>
#include <tuple>
#include <unordered_map>

GLuint VoxelRenderer::prepareRayTraceFromCameraProgram;
GLuint VoxelRenderer::executeRayTraceProgram;
GLuint VoxelRenderer::resetHitInfoProgram;
GLuint VoxelRenderer::displayToWindowProgram;

void VoxelRenderer::remakeTextures()
{
    isSizingDirty = false;
    // This will delete the texture currently bound to this variable, and set the variable equal to 0
    // If the variable is 0, meaning that no texture is bound, then it will do nothing
    glDeleteTextures(1, &rayStartBuffer);
    glDeleteTextures(1, &rayDirectionBuffer);

    glDeleteTextures(1, &rayHitPositionBuffer);
    glDeleteTextures(1, &rayHitNormalBuffer);
    glDeleteTextures(1, &rayHitMaterialBuffer);

    // Create a new texture
    rayStartBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayDirectionBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    rayHitPositionBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayHitNormalBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayHitMaterialBuffer = GraphicsUtils::create3DImage(xSize, ySize, raysPerPixel, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT);
}

void VoxelRenderer::handleDirtySizing()
{
    if (isSizingDirty)
    {
        remakeTextures();
    }
}

VoxelRenderer::VoxelRenderer()
{
    prepareRayTraceFromCameraProgram = ShaderManager::getManager().getComputeProgram("content/PrepareRayTraceFromCamera.compute.glsl");
    executeRayTraceProgram = ShaderManager::getManager().getComputeProgram("content/ExecuteRayTrace.compute.glsl");
    resetHitInfoProgram = ShaderManager::getManager().getComputeProgram("content/ResetHitInfo.compute.glsl");
    displayToWindowProgram = ShaderManager::getManager().getGraphicsProgram("content/ScreenTri.vertex.glsl", "content/DisplayToWindow.frag.glsl");
}

void VoxelRenderer::setResolution(int x, int y)
{
    if (xSize != x || ySize != y)
    {
        isSizingDirty = true;
    }
    xSize = x;
    ySize = y;
}

void VoxelRenderer::setRaysPerPixel(int number)
{
    if (raysPerPixel != number)
    {
        isSizingDirty = true;
    }
    raysPerPixel = number;
}

void VoxelRenderer::prepareRayTraceFromCamera(const Camera& camera)
{
    handleDirtySizing();

    glUseProgram(prepareRayTraceFromCameraProgram);

    glUniform3fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camPosition"), 1, glm::value_ptr(camera.getPosition()));
    glUniform4fv(glGetUniformLocation(prepareRayTraceFromCameraProgram, "camOrientation"), 1, glm::value_ptr(camera.getOrientation()));
    glUniform1f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "horizontalFovTan"), camera.getHorizontalFov());
    glUniform2f(glGetUniformLocation(prepareRayTraceFromCameraProgram, "jitter"), (rand() % 1000) / 1000.f, (rand() % 1000) / 1000.f);

    glBindImageTexture(
        0, // Image unit index (matches binding=0)
        rayStartBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        rayDirectionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

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
        1, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

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

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

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
}

void VoxelRenderer::executeRayTrace(const std::vector<VoxelWorld>& worlds)
{
    handleDirtySizing();

    glUseProgram(executeRayTraceProgram);

    // bind rayStart info
    glBindImageTexture(
        3, // Image unit index (matches binding=0)
        rayStartBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        4, // Image unit index (matches binding=1)
        rayDirectionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

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

    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
    GLuint workGroupsZ = raysPerPixel;

    for (auto& voxelWorld : worlds)
    {
        voxelWorld.bindTextures();

        glUniform3fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldPosition"), 1, glm::value_ptr(voxelWorld.getPosition()));
        glUniform4fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldOrientation"), 1, glm::value_ptr(voxelWorld.getOrientation()));
        glUniform3fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldScale"), 1, glm::value_ptr(voxelWorld.getScale()));

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        voxelWorld.unbindTextures();
    }

    // unbind rayStart info
    glBindImageTexture(
        3, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        4, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

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

    glBindVertexArray(GraphicsUtils::getEmptyVertexArray());

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUseProgram(0);
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
}
