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
    data.writeTo(data);
}
