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
    // data.decodeMaterialMipMap();
    data.clearOccupancy();
    data.clearMaterials();
    {
        generateData();
    }
    data.encodeMaterialMipMap();
    data.writeTo(voxelWorld);
}
