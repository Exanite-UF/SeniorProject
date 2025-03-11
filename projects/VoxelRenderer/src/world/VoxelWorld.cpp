#include <iostream>

#include <glm/gtc/integer.hpp>

#include <src/graphics/GraphicsUtility.h>
#include <src/world/VoxelWorld.h>
#include <src/world/VoxelWorldUtility.h>

VoxelWorld::VoxelWorld(glm::ivec3 size, GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram)
{
    this->makeNoiseComputeProgram = makeNoiseComputeProgram;
    this->makeMipMapComputeProgram = makeMipMapComputeProgram;
    this->assignMaterialComputeProgram = assignMaterialComputeProgram;

    this->currentNoiseTime = 0;

    // Initialize world size and contents
    size = {
        1 << glm::log2(size.x - 1) + 1,
        1 << glm::log2(size.y - 1) + 1,
        1 << glm::log2(size.z - 1) + 1
    };

    setSize(size);
    generateOccupancyAndMipMapsAndMaterials(0, true, 0.6);
}

void VoxelWorld::generateOccupancyAndMipMapsAndMaterials(double deltaTime, bool isRand2, float fillAmount)
{
    generateOccupancyUsingNoise(currentNoiseTime, isRand2, fillAmount);
    updateMipMaps();

    // This calls a shader that hard codes the material values (it is non-essential)
    assignMaterial();

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

std::vector<GLuint> VoxelWorld::getOccupancyMapIndices() const
{
    return occupancyMapIndices;
}

const GraphicsBuffer<uint16_t>& VoxelWorld::getMaterialMap()
{
    return materialMap;
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

void VoxelWorld::assignMaterial()
{
    glUseProgram(assignMaterialComputeProgram);

    this->materialMap.bind(0);

    int sizeX = this->size.x / 2 / (1 << 2);
    int sizeY = this->size.y / 2 / (1 << 2);
    int sizeZ = this->size.z / 2 / (1 << 2);

    GLuint workGroupsX = (sizeX + 8 - 1) / 8; // Ceiling division
    GLuint workGroupsY = (sizeY + 8 - 1) / 8;
    GLuint workGroupsZ = (sizeZ + 8 - 1) / 8;

    glUniform3i(glGetUniformLocation(assignMaterialComputeProgram, "voxelCount"), sizeX, sizeY, sizeZ); // Pass in the resolution of the previous mip map texture

    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);

    // Ensure compute shader completes
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    this->materialMap.unbind();
}

void VoxelWorld::setSize(glm::ivec3 size)
{
    // The size is validated by VoxelWorldUtility below
    this->size = size;

    occupancyMapIndices = VoxelWorldUtility::getOccupancyMapIndices(size);
    this->occupancyMap.setSize(occupancyMapIndices[occupancyMapIndices.size() - 1]);

    this->materialMap.setSize(size.x * size.y * size.z);
}
