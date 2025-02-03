#include "VoxelRenderer.h"
#include "TupleHasher.h"
#include <string>
#include <tuple>
#include <unordered_map>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "ShaderManager.h"
#include <glm/common.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include "VoxelWorld.h"

//TODO: call this function from a utility class
GLuint create3DImage(int width, int height, int depth, GLenum internalFormat, GLenum format, GLenum type)
{
    GLuint img;
    glGenTextures(1, &img);

    glBindTexture(GL_TEXTURE_3D, img);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage3D(
        GL_TEXTURE_3D, 0, internalFormat,
        width, height, depth, // Dimensions for new mip level
        0, format, type, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0);
    return img;
}


void VoxelRenderer::remakeTextures()
{
    isSizingDirty = false;
    //This will delete the texture currently bound to this variable, and set the variable equal to 0
    //If the variable is 0, meaning that no texture is bound, then it will do nothing
    glDeleteTextures(1, &rayStartBuffer);
    glDeleteTextures(1, &rayDirectionBuffer);

    glDeleteTextures(1, &rayHitPositionBuffer);
    glDeleteTextures(1, &rayHitNormalBuffer);
    glDeleteTextures(1, &rayHitMaterialBuffer);

    glDeleteTextures(1, &depthBuffer);
    glDeleteTextures(1, &tempDepthBuffer);

    //Create a new texture
    rayStartBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayDirectionBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    rayHitPositionBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayHitNormalBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    rayHitMaterialBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_R16UI, GL_RED_INTEGER, GL_UNSIGNED_SHORT);

    depthBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_R32F, GL_RED, GL_FLOAT);
    tempDepthBuffer = create3DImage(xSize, ySize, raysPerPixel, GL_R32F, GL_RED, GL_FLOAT);
}

void VoxelRenderer::handleDirtySizing()
{
    if(isSizingDirty){
        remakeTextures();
    }
}

void VoxelRenderer::log(const std::string& value)
{
    std::cout << value + "\n"
              << std::flush;
}

void VoxelRenderer::checkForContentFolder()
{
    if (!std::filesystem::is_directory("content"))
    {
        throw std::runtime_error("Could not find content folder. Is the working directory set correctly?");
    }
    else
    {
        log("Found content folder");
    }
}

void VoxelRenderer::assertIsTrue(const bool condition, const std::string& errorMessage)
{
    if (!condition)
    {
        throw std::runtime_error("Assert failed: " + errorMessage);
    }
}

void VoxelRenderer::runStartupTests()
{
    checkForContentFolder();
    assertIsTrue(NULL == nullptr, "Unsupported compiler: NULL must equal nullptr");

    auto key = std::make_tuple("hello", 1);
    std::unordered_map<std::tuple<std::string, int>, int, TupleHasher<std::tuple<std::string, int>>> map;
    assertIsTrue(!map.contains(key), "Incorrect map implementation: Map should be empty");
    map[key] = 1;
    assertIsTrue(map.contains(key), "Incorrect map implementation: Map should contain key that was inserted");
}

VoxelRenderer::VoxelRenderer()
{
    prepareRayTraceFromCameraProgram = ShaderManager::getManager().getComputeProgram("content/PrepareRayTraceFromCamera.compute.glsl");
    executeRayTraceProgram = ShaderManager::getManager().getComputeProgram("content/ExecuteRayTrace.compute.glsl");
    resetHitInfoProgram = ShaderManager::getManager().getComputeProgram("content/ResetHitInfo.compute.glsl");
}

void VoxelRenderer::setResolution(int x, int y)
{
    xSize = x;
    ySize = y;
    isSizingDirty = true;
}

void VoxelRenderer::setRaysPerPixel(int number)
{
    raysPerPixel = number;
    isSizingDirty = true;
}

void VoxelRenderer::prepateRayTraceFromCamera(const Camera& camera)
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



    //Reset the hit info
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

    //bind rayStart info
    glBindImageTexture(
        5, // Image unit index (matches binding=0)
        rayStartBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        6, // Image unit index (matches binding=1)
        rayDirectionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );
    
    //bind hit info
    glBindImageTexture(
        7, // Image unit index (matches binding=0)
        rayHitPositionBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        8, // Image unit index (matches binding=0)
        rayHitNormalBuffer, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        9, // Image unit index (matches binding=0)
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

    for(auto& voxelWorld : worlds){
        voxelWorld.bindVoxelTextures();

        glUniform3fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldPosition"), 1, glm::value_ptr(voxelWorld.getPosition()));
        glUniform4fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldOrientation"), 1, glm::value_ptr(voxelWorld.getOrientation()));
        glUniform3fv(glGetUniformLocation(executeRayTraceProgram, "voxelWorldScale"), 1, glm::value_ptr(voxelWorld.getScale()));

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        voxelWorld.unbindVoxelTexture();
    }

    //unbind rayStart info
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
        6, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );
    
    //unbind hit info
    glBindImageTexture(
        7, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        8, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA32F // Format
    );

    glBindImageTexture(
        9, // Image unit index (matches binding=0)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_R16UI // Format
    );

    glUseProgram(0);
}
