#include <iostream>

#include <glm/gtc/integer.hpp>

#include <src/graphics/GraphicsUtility.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkUtility.h>

#include "VoxelChunkResources.h"

VoxelChunk::VoxelChunk(glm::ivec3 size, bool shouldGeneratePlaceholderData)
{
    this->currentNoiseTime = 0;

    setSize(size);

    if (shouldGeneratePlaceholderData)
    {
        // Generates initial occupancy map data
        generatePlaceholderData(0, true, 0.6);

        // Generates initial material map
        generatePlaceholderMaterialMap();
    }
}

glm::ivec3 VoxelChunk::getSize() const
{
    return size;
}

const GraphicsBuffer<uint8_t>& VoxelChunk::getOccupancyMap()
{
    return occupancyMap;
}

std::vector<GLuint> VoxelChunk::getOccupancyMapIndices() const
{
    return occupancyMapIndices;
}

const GraphicsBuffer<uint16_t>& VoxelChunk::getMaterialMap()
{
    return materialMap;
}

void VoxelChunk::setSize(glm::ivec3 size)
{
    ZoneScoped;

    // Chunk size must be a power of 2
    this->size = {
        1 << glm::log2(size.x - 1) + 1,
        1 << glm::log2(size.y - 1) + 1,
        1 << glm::log2(size.z - 1) + 1
    };

    occupancyMapIndices = VoxelChunkUtility::getOccupancyMapIndices(size);
    this->occupancyMap.setSize(occupancyMapIndices[occupancyMapIndices.size() - 1]);

    this->materialMap.setSize(size.x * size.y * size.z);
}

void VoxelChunk::generateNoiseOccupancyMap(double noiseTime, bool useRandomNoise, float fillAmount)
{
    auto& voxelWorldManager = VoxelChunkResources::getInstance();
    auto makeNoiseComputeProgram = voxelWorldManager.makeNoiseComputeProgram;

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
    glUniform1i(glGetUniformLocation(makeNoiseComputeProgram, "isRand2"), useRandomNoise);

    glDispatchCompute(workgroupCountX, 1, 1);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelChunk::updateMipMaps()
{
    auto& voxelWorldManager = VoxelChunkResources::getInstance();
    auto makeMipMapComputeProgram = voxelWorldManager.makeMipMapComputeProgram;

    glUseProgram(makeMipMapComputeProgram);

    occupancyMap.bind(0);

    for (int i = 0; i < occupancyMapIndices.size() - 1; i++)
    {
        int sizeX = this->size.x / 2 / (1 << (2 * i)); // This needs the size of the previous mipmap (The divisions to this: voxel size -> size of first texture -> size of previous mipmap)
        int sizeY = this->size.y / 2 / (1 << (2 * i));
        int sizeZ = this->size.z / 2 / (1 << (2 * i));

        int occupancyMapUintCount = ((sizeX * sizeY * sizeZ) / 64) / 4; // Number of bytes in next mip map / 4 to get number of uints
        int workgroupSize = 256; // Each workgroup handles 256 uints (a multiple of 32 is optimal) //TODO: find best workgroup size for performance
        GLuint workGroupCount = (occupancyMapUintCount + workgroupSize - 1) / workgroupSize;

        glUniform3i(glGetUniformLocation(makeMipMapComputeProgram, "resolution"), sizeX, sizeY, sizeZ); // Pass in the resolution of the previous mip map texture
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "previousStartByte"), occupancyMapIndices[i]); // Pass in the starting byte location of the previous mip map texture
        glUniform1ui(glGetUniformLocation(makeMipMapComputeProgram, "nextStartByte"), occupancyMapIndices[i + 1]); // Pass in the starting byte location of the next mip map texture.

        glDispatchCompute(workGroupCount, 1, 1);

        // Ensure compute shader completes
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Ensure writes are finished
    }

    occupancyMap.unbind();
    glUseProgram(0);
}

void VoxelChunk::generatePlaceholderMaterialMap()
{
    auto& voxelWorldManager = VoxelChunkResources::getInstance();
    auto assignMaterialComputeProgram = voxelWorldManager.assignMaterialComputeProgram;

    glUseProgram(assignMaterialComputeProgram);

    this->materialMap.bind(0);

    GLuint workGroupsX = (size.x + 4 - 1) / 4; // Ceiling division
    GLuint workGroupsY = (size.y + 8 - 1) / 8;
    GLuint workGroupsZ = (size.z + 8 - 1) / 8;

    glUniform3i(glGetUniformLocation(assignMaterialComputeProgram, "voxelCount"), size.x, size.y, size.z); // Pass in the resolution of the previous mip map texture

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    this->materialMap.unbind();
}

void VoxelChunk::generatePlaceholderData(double deltaTime, bool useRandomNoise, float fillAmount)
{
    generateNoiseOccupancyMap(currentNoiseTime, useRandomNoise, fillAmount);
    updateMipMaps();

    // Updating noise after generating makes the initial generation independent to framerate
    this->currentNoiseTime += deltaTime;
}

void VoxelChunk::bindBuffers(int occupancyMapIndex, int materialMapIndex)
{
    occupancyMap.bind(occupancyMapIndex);
    materialMap.bind(materialMapIndex);
}

void VoxelChunk::unbindBuffers() const
{
    occupancyMap.unbind();
    materialMap.unbind();
}
