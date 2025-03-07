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

    // TODO: Remove automatic copy, decode, encode when API has stabilized
    // data.copyFrom(voxelWorld);
    // data.decodePaletteMap();
    data.clearOccupancyMap();
    data.clearMaterialMap();
    {
        generateData();
    }
    data.encodePaletteMap();
    data.writeTo(voxelWorld);
}
