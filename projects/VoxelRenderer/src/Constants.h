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
        static constexpr uint32_t minSizePerAxis = 32;
        static constexpr uint8_t maxOccupancyMapLayerCount = 10; // 1 base layer + 9 mipmap layers
        static constexpr uint8_t materialMapLayerCount = 3;
    };
};
