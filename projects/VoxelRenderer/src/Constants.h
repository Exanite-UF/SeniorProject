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

        // Directly uploading a large chunk of memory to the GPU blocks the rendering thread,
        // even when the upload is done from a separate OpenGL context on a separate thread
        // This is used to break up the upload into chunks
        static constexpr int chunkUploadSleepTimeMs = 10;
        static constexpr uint64_t maxChunkUploadSizeBytes = 32 * 1024 * 1024;
    };

    class VoxelChunkComponent
    {
    public:
        static constexpr glm::ivec3 chunkSize = glm::ivec3(16, 16, 16);
    };

    class GameObject
    {
    public:
        static constexpr bool isEventLoggingEnabled = false;
        static constexpr bool isUpdateEventLoggingEnabled = false;
    };
};
