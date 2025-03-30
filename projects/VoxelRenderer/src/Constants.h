#pragma once

#include <glm/vec3.hpp>

// A class containing commonly used constants.
// These constants are stored here to improve visibility. Otherwise, it's easy to accidentally introduce duplicate constants.
class Constants
{
public:
    class VoxelChunk
    {
    public:
        // The max number of mip levels in our occupancy map
        // This is arbitrary, but 10 is very large amount
        // 10 means that the max chunk size is 2^10 = 4096
        static constexpr uint8_t maxOccupancyMapLayerCount = 10; // 1 base layer + 9 mipmap layers

        // The max number of material definitions
        static constexpr uint32_t maxMaterialCount = 65536;
    };

    class VoxelChunkComponent
    {
    public:
        static constexpr glm::ivec3 chunkSize = glm::ivec3(512, 512, 512);
    };

    class VoxelChunkManager
    {
    public:
        static constexpr int maxChunkModificationThreads = 2;
    };

    class GameObject
    {
    public:
        static constexpr bool isEventLoggingEnabled = false;
        static constexpr bool isUpdateEventLoggingEnabled = false;
    };
};
