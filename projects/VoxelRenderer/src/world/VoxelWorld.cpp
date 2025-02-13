#include <chrono>
#include <iostream>

#include <src/graphics/GraphicsUtility.h>
#include <src/world/VoxelWorld.h>

VoxelWorld::VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram)
{
    this->makeNoiseComputeProgram = makeNoiseComputeProgram;
    this->makeMipMapComputeProgram = makeMipMapComputeProgram;
    this->assignMaterialComputeProgram = assignMaterialComputeProgram;

    this->currentNoiseTime = 0;

    // Make and fill the buffers
    setSize({ 512, 512, 512 });
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

    assignMaterial(materialMap, 0);
    assignMaterial(materialMap, 1);
    assignMaterial(materialMap, 2);
}

void VoxelWorld::bindTextures(int occupancyMap, int materialMap)
{
    this->occupancyMap.bind(occupancyMap);
    this->materialMap.bind(materialMap);
}

void VoxelWorld::unbindTextures() const
{
    this->occupancyMap.unbind();
    this->materialMap.unbind();
}

glm::ivec3 VoxelWorld::getSize() const
{
    return size;
}

glm::vec3 VoxelWorld::getPosition() const
{
    return transform.position;
}

glm::vec3 VoxelWorld::getScale() const
{
    return transform.scale;
}

glm::quat VoxelWorld::getRotation() const
{
    return transform.rotation;
}

int VoxelWorld::getMipMapTextureCount() const
{
    return mipMapTextureCount;
}

std::array<GLuint, 10> VoxelWorld::getMipMapStartIndices() const
{

    return mipMapStartIndices;
}

std::array<GLuint, 3> VoxelWorld::getMaterialStartIndices() const
{
    return materialStartIndices;
}

void VoxelWorld::makeNoise(ShaderByteBuffer& occupancyMap, double noiseTime, bool isRand2, float fillAmount)
{
    glUseProgram(makeNoiseComputeProgram);

    // Bind output texture to image unit 0 (write-only)
    occupancyMap.bind(0);

    int occupancyMapVoxelCount = size.x * size.y * size.z;
    int occupancyMapUintCount = occupancyMapVoxelCount / 32; // This is two divisions: by 8 and by 4

    int workgroupSize = 256; // Each workgroup handles 256 uints
    int workgroupCountX = (occupancyMapUintCount + workgroupSize - 1) / workgroupSize; // Ceiling division;

    glUniform3i(glGetUniformLocation(makeNoiseComputeProgram, "resolution"), size.x / 2, size.y / 2, size.z / 2);
    glUniform1f(glGetUniformLocation(makeNoiseComputeProgram, "time"), noiseTime);
    glUniform1f(glGetUniformLocation(makeNoiseComputeProgram, "fillAmount"), fillAmount);
    glUniform1i(glGetUniformLocation(makeNoiseComputeProgram, "isRand2"), isRand2);

    glDispatchCompute(workgroupCountX, 1, 1);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelWorld::makeMipMaps(ShaderByteBuffer& occupancyMap)
{
    glUseProgram(makeMipMapComputeProgram);

    occupancyMap.bind(0);

    for (int i = 0; i < mipMapTextureCount; i++)
    {
        // TODO: Use ivec3 here
        int sizeX = this->size.x / 2 / (1 << (2 * i)); // This needs the size of the previous mipmap (The divisions to this: voxel size -> size of first texture -> size of previous mipmap)
        int sizeY = this->size.y / 2 / (1 << (2 * i));
        int sizeZ = this->size.z / 2 / (1 << (2 * i));

        int occupancyMapUintCount = ((sizeX * sizeY * sizeZ) / 64) / 4; // Number of bytes in next mip map / 4 to get number of uints
        int workgroupSize = 256; // Each workgroup handles 256 uints (a multiple of 32 is optimal) //TODO: find best workgroup size for performance
        GLuint workGroupCount = (occupancyMapUintCount + workgroupSize - 1) / workgroupSize;

        glUniform3i(glGetUniformLocation(makeMipMapComputeProgram, "resolution"), sizeX, sizeY, sizeZ); // Pass in the resolution of the previous mip map texture
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "previousStartByte"), mipMapStartIndices[i]); // Pass in the starting byte location of the previous mip map texture
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "nextStartByte"), mipMapStartIndices[i + 1]); // Pass in the starting byte location of the next mip map texture.

        glDispatchCompute(workGroupCount, 1, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // Ensure writes are finished
    }

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelWorld::assignMaterial(ShaderByteBuffer& materialMap, int level)
{
    glUseProgram(assignMaterialComputeProgram);

    this->materialMap.bind(0);

    int sizeX = this->size.x / 2 / (1 << (2 * level)); // This needs the size of the previous mipmap (The divisions to this: voxel size -> size of first texture -> size of previous mipmap)
    int sizeY = this->size.y / 2 / (1 << (2 * level));
    int sizeZ = this->size.z / 2 / (1 << (2 * level));

    GLuint workGroupsX = (sizeX + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (sizeY + 8 - 1) / 8;
    GLuint workGroupsZ = (sizeZ + 8 - 1) / 8;

    glUniform3i(glGetUniformLocation(assignMaterialComputeProgram, "resolution"), sizeX, sizeY, sizeZ); // Pass in the resolution of the previous mip map texture
    glUniform1ui(glGetUniformLocation(assignMaterialComputeProgram, "materialStartIndex"), materialStartIndices[level]); // Pass in the resolution of the previous mip map texture

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    this->materialMap.unbind();
}

void VoxelWorld::setSize(glm::ivec3 size)
{
    if (size.x < minSize.x || size.y < minSize.y || size.z < minSize.z)
    {
        std::cout << "The minimum size of a voxel world along an axis is 32." << std::endl;
        throw "The minimum size of a voxel world along an axis is 32."; // TODO: Throw something better
    }

    this->size = size;

    mipMapTextureCount = std::floor(std::log2(std::min(std::min(size.x, size.y), size.z) / 4 /*This is a 4 and not a 2, because the mip map generation will break if the top level mip map has side length 1. This prevents that from occuring.*/) / 2); // This is what the name says it is

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
        bytesOfOccupancyMap += size.x * size.y * size.z / 8 / divisor;
    }

    this->occupancyMap.setSize(bytesOfOccupancyMap);

    std::uint64_t bytesOfMaterialMap = 0;
    for (int i = 0; i < 3; i++)
    {
        std::uint64_t divisor = (1 << (2 * i));
        divisor *= divisor * divisor; // Cube the divisor
        materialStartIndices[i] = bytesOfMaterialMap;
        bytesOfMaterialMap += 4 * size.x * size.y * size.z / 8 / divisor;
    }

    this->materialMap.setSize(bytesOfMaterialMap);
}
