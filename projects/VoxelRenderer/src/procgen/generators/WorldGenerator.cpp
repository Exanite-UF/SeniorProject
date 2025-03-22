#include <src/procgen/generators/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelChunkComponent.h>
#include <src/world/VoxelChunkData.h>

WorldGenerator::WorldGenerator() = default;

void WorldGenerator::generate(VoxelChunkData& data)
{
    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData(data);
    }
}

void WorldGenerator::generate(VoxelChunkComponent& chunk)
{
    MeasureElapsedTimeScope scope("WorldGenerator::generate");

    VoxelChunkData& data = chunk.getChunkData();

    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData(data);
    }
    if (chunk.getExistsOnGpu())
    {
        data.writeTo(*chunk.getChunk());
    }
}

void WorldGenerator::setChunkSize(const glm::ivec3& chunkSize)
{
    this->chunkSize = chunkSize;
}

const glm::ivec3& WorldGenerator::getChunkSize()
{
    return chunkSize;
}

void WorldGenerator::setChunkPosition(const glm::ivec3& chunkPosition)
{
    this->chunkPosition = chunkPosition;
}

const glm::ivec3& WorldGenerator::getChunkPosition()
{
    return chunkPosition;
}
