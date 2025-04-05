#include <tracy/Tracy.hpp>

#include <src/procgen/generators/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelChunkComponent.h>
#include <src/world/VoxelChunkData.h>

WorldGenerator::WorldGenerator() = default;

void WorldGenerator::generate(VoxelChunkData& data)
{
    ZoneScoped;

    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData(data);
    }
}

void WorldGenerator::generate(VoxelChunkComponent& chunk)
{
    ZoneScoped;

    MeasureElapsedTimeScope scope("WorldGenerator::generate");

    std::lock_guard lock(chunk.getMutex());

    VoxelChunkData& data = chunk.getRawChunkData();

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

const glm::ivec3& WorldGenerator::getChunkSize() const
{
    return chunkSize;
}

void WorldGenerator::setChunkPosition(const glm::ivec3& chunkPosition)
{
    this->chunkPosition = chunkPosition;
}

const glm::ivec3& WorldGenerator::getChunkPosition() const
{
    return chunkPosition;
}
