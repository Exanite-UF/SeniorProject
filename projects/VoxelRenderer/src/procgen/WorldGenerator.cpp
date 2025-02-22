#include <src/procgen/WorldGenerator.h>
#include <imgui/imgui.h>
#include <src/PerlinNoise.hpp>
#include <src/world/VoxelWorldData.h>


WorldGenerator::WorldGenerator(glm::ivec3 worldSize)
{
    data.setSize(worldSize);
}

void WorldGenerator::generate(VoxelWorld& voxelWorld)
{
    //TODO: Remove copyFrom, and also set material. Otherwise voxels are black. 
    data.copyFrom(voxelWorld);
    data.clearOccupancy();
    generateData();
    data.writeTo(voxelWorld);
}