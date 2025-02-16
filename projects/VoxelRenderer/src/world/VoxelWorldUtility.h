#pragma once

#include <tuple>
#include <vector>

#include <src/Constants.h>

class VoxelWorldUtility
{
public:
    // First value will always be 0
    // Last value will be the size of the occupancy map in bytes
    // Values in between represent the start indexes of each mipmap layer in bytes
    //
    // The total mipmap layers is result.size() - 2
    // The total layers is result.size() - 1
    //
    // The max size of the returned vector is Constants::VoxelWorld::maxOccupancyMapLayerCount + 1
    static std::vector<uint32_t> calculateOccupancyMapIndices(glm::ivec3 size);

    // First value will always be 0
    // Last value will be the size of the material map in bytes
    // Values in between represent the start indexes of each mipmap layer in bytes
    //
    // The total layers is result.size() - 1
    //
    // The exact size of the returned array is Constants::VoxelWorld::materialMapLayerCount + 1
    static std::array<uint32_t, Constants::VoxelWorld::materialMapLayerCount + 1> calculateMaterialMapIndices(glm::ivec3 size);
};
