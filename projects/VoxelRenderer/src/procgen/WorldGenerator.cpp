#include <src/procgen/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelWorldData.h>

WorldGenerator::WorldGenerator(glm::ivec3 worldSize)
{
    data.setSize(worldSize);
}

void WorldGenerator::generate(VoxelWorld& voxelWorld)
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
