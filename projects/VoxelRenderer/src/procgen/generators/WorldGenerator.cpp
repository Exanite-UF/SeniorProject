#include <src/procgen/generators/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelChunkData.h>

WorldGenerator::WorldGenerator(glm::ivec3 worldSize)
{
    data.setSize(worldSize);
}

void WorldGenerator::generate(VoxelChunk& voxelWorld)
{
    MeasureElapsedTimeScope scope("WorldGenerator::generate");

    // data.copyFrom(voxelWorld);
    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData();
    }
    data.writeTo(voxelWorld);
}
