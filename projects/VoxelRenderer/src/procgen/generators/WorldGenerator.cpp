#include <src/procgen/generators/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
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

void WorldGenerator::generate(VoxelChunk& chunk)
{
    MeasureElapsedTimeScope scope("WorldGenerator::generate");

    VoxelChunkData data(chunk.getSize());

    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData(data);
    }
    data.writeTo(chunk);
}
