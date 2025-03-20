#include <src/procgen/generators/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelChunkData.h>

WorldGenerator::WorldGenerator(glm::ivec3 chunkSize)
{
    data.setSize(chunkSize);
}

void WorldGenerator::generate(VoxelChunk& chunk)
{
    MeasureElapsedTimeScope scope("WorldGenerator::generate");

    // data.copyFrom(chunk);
    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData();
    }
    data.writeTo(chunk);
}
