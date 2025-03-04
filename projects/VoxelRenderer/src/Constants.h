#pragma once

#include <glm/vec3.hpp>

// A class containing commonly used constants.
// These constants are stored here to improve visibility. Otherwise, it's easy to accidentally introduce a duplicate constant.
class Constants
{
public:
    class VoxelWorld
    {
    public:
        // This is equivalent to 2*4*4.
        // The more occupancy mip levels we have, the higher this limit is.
        // 2 is from storing voxels in 2x2x2 cells.
        // 4 is from dividing by 4 twice to generate our 3 material mip levels.
        static constexpr uint32_t minSizePerAxis = 32;

        // The max number of mip levels in our occupancy map.
        // This is arbitrary, but 10 is very large amount.
        // 10 means that the max chunk size is 2^10 = 4096.
        static constexpr uint8_t maxOccupancyMapLayerCount = 10; // 1 base layer + 9 mipmap layers

        // The max number of mip levels in our material map.
        static constexpr uint8_t materialMapLayerCount = 3;

        // The max number of material IDs. Material IDs are first mapped to indices representing material instances.
        // This is equivalent to 2^12 or 2^(3*4). This is because we have 4 bits per material mip level, and we have 3 mip levels.
        static constexpr uint32_t palette0Count = 4096;

        // This is equivalent to 2^8.
        static constexpr uint32_t palette1Count = 256;

        // This is equivalent to 2^4.
        static constexpr uint32_t palette2Count = 16;

        // This is a fake material palette layer used to make code easier to write.
        // This is equivalent to 2^0.
        static constexpr uint32_t palette3Count = 1;

        // The max number of material instances.
        static constexpr uint32_t materialCount = 512;

        // The max number of materials or palettes per material palette region.
        // This is equivalent to 2^4. This is because we have 4 bits per material mip level.
        static constexpr uint32_t materialsPerRegion = 16;
    };
};
