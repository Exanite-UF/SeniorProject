#include "VoxelWorld.h"

#include "GraphicsUtils.h"
#include <chrono>
#include <iostream>
#include <unistd.h>

VoxelWorld::VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram)
{
    this->makeNoiseComputeProgram = makeNoiseComputeProgram;
    this->makeMipMapComputeProgram = makeMipMapComputeProgram;
    this->assignMaterialComputeProgram = assignMaterialComputeProgram;

    this->currentNoiseTime = 0;

    // Make and fill the buffers
    xSize = 512;
    ySize = 512;
    zSize = 512;

    mipMapTextureCount = std::floor(std::log2(std::min(std::min(xSize, ySize), zSize) / 2) / 2);

    // No more than 9 mip maps can be made from the occupancy map
    if (mipMapTextureCount > 9)
    {
        mipMapTextureCount = 9;
    }
    // This should be the exact number of bytes that the occupancy map and all its mip maps take up
    std::uint64_t bytesOfOccupancyMap = 0;
    for (int i = 0; i <= mipMapTextureCount; i++)
    {
        std::uint64_t divisor = (1 << (2 * i));
        divisor *= divisor * divisor; // Cube the divisor
        mipMapStartIndices[i] = bytesOfOccupancyMap;

        bytesOfOccupancyMap += xSize * ySize * zSize / 8 / divisor;
    }

    std::cout << mipMapTextureCount << std::endl;

    this->occupancyMap.setSize(bytesOfOccupancyMap);
    // this->occupancyMap = GraphicsUtils::create3DImage(width / 2, height / 2, depth / 2, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    // this->mipMap1 = GraphicsUtils::create3DImage(width / 8, height / 8, depth / 8, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    // this->mipMap2 = GraphicsUtils::create3DImage(width / 32, height / 32, depth / 32, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    // this->mipMap3 = GraphicsUtils::create3DImage(width / 128, height / 128, depth / 128, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);
    // this->mipMap4 = GraphicsUtils::create3DImage(width / 512, height / 512, depth / 512, GL_RGBA8UI, GL_RGBA_INTEGER, GL_UNSIGNED_BYTE);

    generateFromNoise(0, true, 0.6);

    // assignMaterial(occupancyMap);
    // assignMaterial(mipMap1);
    // assignMaterial(mipMap2);
}

void VoxelWorld::generateFromNoise(double deltaTime, bool isRand2, float fillAmount) // bool isRand2, float fillAmount
{
    this->currentNoiseTime += deltaTime;

    makeNoise(occupancyMap, currentNoiseTime, true, 0.6);
    makeMipMaps(occupancyMap);
    // makeMipMap(occupancyMap, mipMap1);
    // makeMipMap(mipMap1, mipMap2);
    // makeMipMap(mipMap2, mipMap3);
    // makeMipMap(mipMap3, mipMap4);
}

void VoxelWorld::bindTextures(int occupancyMap)
{
    this->occupancyMap.bind(occupancyMap);
}

void VoxelWorld::unbindTextures() const
{
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        1, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        2, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        3, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );

    glBindImageTexture(
        4, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_ONLY, // Access qualifier
        GL_RGBA8UI // Format
    );
}

glm::ivec3 VoxelWorld::getSize() const
{
    return glm::ivec3(zSize, ySize, zSize);
}

glm::vec3 VoxelWorld::getPosition() const
{
    return position;
}

glm::vec3 VoxelWorld::getScale() const
{
    return scale;
}

glm::quat VoxelWorld::getOrientation() const
{
    return orientation;
}

int VoxelWorld::getMipMapTextureCount() const
{
    return mipMapTextureCount;
}

std::array<GLuint, 10> VoxelWorld::getMipMapStartIndices() const
{

    return mipMapStartIndices;
}

void VoxelWorld::makeNoise(ShaderByteBuffer& occupancyMap, double noiseTime, bool isRand2, float fillAmount)
{
    glUseProgram(makeNoiseComputeProgram);

    // Bind output texture to image unit 1 (write-only)
    occupancyMap.bind(0);

    int xSize = this->xSize / 2; // Everything here needs the size of the texture, not the number of voxels
    int ySize = this->ySize / 2; // Everything here needs the size of the texture, not the number of voxels
    int zSize = this->zSize / 2; // Everything here needs the size of the texture, not the number of voxels

    GLuint workGroupsX = (xSize + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (ySize + 8 - 1) / 8;
    GLuint workGroupsZ = (zSize + 8 - 1) / 8;

    glUniform3i(glGetUniformLocation(makeNoiseComputeProgram, "resolution"), xSize, ySize, zSize);
    glUniform1f(glGetUniformLocation(makeNoiseComputeProgram, "time"), noiseTime);
    glUniform1f(glGetUniformLocation(makeNoiseComputeProgram, "fillAmount"), fillAmount);
    glUniform1i(glGetUniformLocation(makeNoiseComputeProgram, "isRand2"), isRand2);

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelWorld::makeMipMaps(ShaderByteBuffer& occupancyMap)
{
    glUseProgram(makeMipMapComputeProgram);

    // Bind output texture to image unit 1 (write-only)
    occupancyMap.bind(0);

    for (int i = 0; i < mipMapTextureCount; i++)
    {
        int xSize = this->xSize / 2 / (1 << (2 * i)); // This needs the size of the previous mipmap (The divisions to this: voxel size -> size of first texture -> size of previous mipmap)
        int ySize = this->ySize / 2 / (1 << (2 * i));
        int zSize = this->zSize / 2 / (1 << (2 * i));

        // Work groups needs to be based on the size of the next mipmap
        GLuint workGroupsX = (xSize / 4 + 8 - 1) / 8; // Ceiling division
        GLuint workGroupsY = (ySize / 4 + 8 - 1) / 8;
        GLuint workGroupsZ = (zSize / 4 + 8 - 1) / 8;

        glUniform3i(glGetUniformLocation(makeMipMapComputeProgram, "resolution"), xSize, ySize, zSize);

        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "previousStartByte"), mipMapStartIndices[i]);
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "nextStartByte"), mipMapStartIndices[i + 1]);
        // std::cout <<"A: "<< mipMapStarts[i] << std::endl;
        // std::cout <<"B: "<< mipMapStarts[i + 1] << std::endl;
        // std::cout << std::endl;

        glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelWorld::assignMaterial(GLuint image3D)
{
    glUseProgram(assignMaterialComputeProgram);

    // Bind output texture to image unit 1 (write-only)
    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        image3D, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA8UI // Format
    );

    int outputWidth, outputHeight, outputDepth;

    glBindTexture(GL_TEXTURE_3D, image3D);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &outputWidth);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_HEIGHT, &outputHeight);
    glGetTexLevelParameteriv(GL_TEXTURE_3D, 0, GL_TEXTURE_DEPTH, &outputDepth);
    glBindTexture(GL_TEXTURE_3D, 0);

    GLuint workGroupsX = (outputWidth + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (outputHeight + 8 - 1) / 8;
    GLuint workGroupsZ = (outputDepth + 8 - 1) / 8;

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindImageTexture(
        0, // Image unit index (matches binding=1)
        0, // Texture ID
        0, // Mip level
        GL_TRUE, // Layered (true for 3D textures)
        0, // Layer (ignored for 3D)
        GL_READ_WRITE, // Access qualifier
        GL_RGBA8UI // Format
    );
}
