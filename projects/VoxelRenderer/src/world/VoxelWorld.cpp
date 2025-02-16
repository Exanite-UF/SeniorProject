#include <iostream>

#include <src/graphics/GraphicsUtility.h>
#include <src/world/VoxelWorld.h>

VoxelWorld::VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram)
{
    this->makeNoiseComputeProgram = makeNoiseComputeProgram;
    this->makeMipMapComputeProgram = makeMipMapComputeProgram;
    this->assignMaterialComputeProgram = assignMaterialComputeProgram;

    this->currentNoiseTime = 0;

    // Initialize world size and contents
    setSize({ 512, 512, 512 });
    generateOccupancyAndMipMapsAndMaterials(0, true, 0.6);
}

void VoxelWorld::generateOccupancyAndMipMapsAndMaterials(double deltaTime, bool isRand2, float fillAmount)
{
    generateOccupancyUsingNoise(currentNoiseTime, true, 0.6);
    updateMipMaps();

    assignMaterial(0);
    assignMaterial(1);
    assignMaterial(2);

    // Updating noise after generating makes the initial generation independent to framerate
    this->currentNoiseTime += deltaTime;
}

void VoxelWorld::bindBuffers(int occupancyMapIndex, int materialMapIndex)
{
    occupancyMap.bind(occupancyMapIndex);
    materialMap.bind(materialMapIndex);
}

void VoxelWorld::unbindBuffers() const
{
    occupancyMap.unbind();
    materialMap.unbind();
}

glm::ivec3 VoxelWorld::getSize() const
{
    return size;
}

const GraphicsBuffer<uint8_t>& VoxelWorld::getOccupancyMap()
{
    return occupancyMap;
}

std::vector<GLuint> VoxelWorld::getOccupancyStartIndices() const
{
    return occupancyStartIndices;
}

const GraphicsBuffer<uint8_t>& VoxelWorld::getMaterialMap()
{
    return materialMap;
}

std::array<GLuint, Constants::VoxelWorld::materialMapLayerCount> VoxelWorld::getMaterialStartIndices() const
{
    return materialStartIndices;
}

void VoxelWorld::generateOccupancyUsingNoise(double noiseTime, bool isRand2, float fillAmount)
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

void VoxelWorld::updateMipMaps()
{
    glUseProgram(makeMipMapComputeProgram);

    occupancyMap.bind(0);

    for (int i = 0; i < occupancyStartIndices.size(); i++)
    {
        // TODO: Use ivec3 here
        int sizeX = this->size.x / 2 / (1 << (2 * i)); // This needs the size of the previous mipmap (The divisions to this: voxel size -> size of first texture -> size of previous mipmap)
        int sizeY = this->size.y / 2 / (1 << (2 * i));
        int sizeZ = this->size.z / 2 / (1 << (2 * i));

        int occupancyMapUintCount = ((sizeX * sizeY * sizeZ) / 64) / 4; // Number of bytes in next mip map / 4 to get number of uints
        int workgroupSize = 256; // Each workgroup handles 256 uints (a multiple of 32 is optimal) //TODO: find best workgroup size for performance
        GLuint workGroupCount = (occupancyMapUintCount + workgroupSize - 1) / workgroupSize;

        glUniform3i(glGetUniformLocation(makeMipMapComputeProgram, "resolution"), sizeX, sizeY, sizeZ); // Pass in the resolution of the previous mip map texture
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "previousStartByte"), occupancyStartIndices[i]); // Pass in the starting byte location of the previous mip map texture
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "nextStartByte"), occupancyStartIndices[i + 1]); // Pass in the starting byte location of the next mip map texture.

        glDispatchCompute(workGroupCount, 1, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_ALL_BARRIER_BITS); // Ensure writes are finished
    }

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelWorld::assignMaterial(int level)
{
    glUseProgram(assignMaterialComputeProgram);

    this->materialMap.bind(0);

    int sizeX = this->size.x / 2 / (1 << (2 * level)); // TODO: Is this correct? -> This needs the size of the previous mipmap (The divisions to this: voxel size -> size of first texture -> size of previous mipmap)
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
    constexpr auto minSize = Constants::VoxelWorld::minSize;
    if (size.x < minSize.x || size.y < minSize.y || size.z < minSize.z)
    {
        throw std::runtime_error("The minimum size of a voxel world along an axis is 32.");
    }

    this->size = size;

    uint8_t layerCount = 1 + std::floor(std::log2(std::min(std::min(size.x, size.y), size.z) / 4 /*This is a 4 and not a 2, because the mip map generation will break if the top level mip map has side length 1. This prevents that from occuring.*/) / 2); // This is what the name says it is
    layerCount = glm::min(layerCount, Constants::VoxelWorld::maxOccupancyMapLayerCount); // Limit the max number of mip maps

    occupancyStartIndices.resize(layerCount);

    // This should be the exact number of bytes that the occupancy map and all its mip maps take up
    std::uint64_t occupancyMapByteCount = 0;
    for (int i = 0; i < layerCount; i++)
    {
        std::uint64_t divisor = (1 << (2 * i));
        divisor *= divisor * divisor; // Cube the divisor
        occupancyStartIndices[i] = occupancyMapByteCount;
        occupancyMapByteCount += size.x * size.y * size.z / 8 / divisor;
    }

    this->occupancyMap.setSize(occupancyMapByteCount);

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
