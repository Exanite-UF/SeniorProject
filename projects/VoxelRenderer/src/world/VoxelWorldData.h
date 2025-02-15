#pragma once

#include <src/world/VoxelWorld.h>

class VoxelWorldData
{
private:
    std::vector<uint8_t> data;

public:
    void copyFrom(const VoxelWorld& world);
};
