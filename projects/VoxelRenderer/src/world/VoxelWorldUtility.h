#pragma once

#include <tuple>
#include <vector>

#include <src/Constants.h>

class VoxelWorldUtility
{
public:
    // First value will always be 0
    // Last value will be the size of the occupancy map
    // Values in between represent the start indexes of each mipmap layer in bytes
    //
    // The total mipmap layers is result.size() - 2
    // The total layers is result.size() - 1
    static std::vector<uint32_t> calculateOccupancyMapIndices(glm::ivec3 size);

    static std::array<uint32_t, Constants::VoxelWorld::materialMapLayerCount + 1> calculateMaterialMapIndices(glm::ivec3 size);
};
