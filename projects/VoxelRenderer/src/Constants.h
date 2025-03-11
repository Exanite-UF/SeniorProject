#pragma once

#include <glm/vec3.hpp>

// A class containing commonly used constants.
// These constants are stored here to improve visibility. Otherwise, it's easy to accidentally introduce duplicate constants.
class Constants
{
public:
    class VoxelWorld
    {
    public:
        // This is equivalent to 2*4*4
        // The more occupancy mip levels we have, the higher this limit is
        // 2 is from storing voxels in 2x2x2 cells
        // 4 is from dividing by 4 twice to generate our 3 material mip levels
        static constexpr uint32_t minSizePerAxis = 32;

        // The max number of mip levels in our occupancy map
        // This is arbitrary, but 10 is very large amount
        // 10 means that the max chunk size is 2^10 = 4096
        static constexpr uint8_t maxOccupancyMapLayerCount = 10; // 1 base layer + 9 mipmap layers

        // The max number of material definitions
        static constexpr uint32_t materialCount = 65536;
    };
};
