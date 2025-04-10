#pragma once

#include <vector>

#include <src/Constants.h>

class VoxelChunkUtility
{
public:
    // First value will always be 0
    // Last value will be the size of the occupancy map in bytes
    // Values in between represent the start indexes of each mipmap layer in bytes
    //
    // The total mipmap layers is result.size() - 2
    // The total layers is result.size() - 1
    //
    // The max size of the returned vector is Constants::VoxelChunk::maxOccupancyMapLayerCount + 1
    static std::vector<uint32_t> getOccupancyMapIndices(const glm::ivec3& size);

    [[nodiscard]] static bool isValidPosition(const glm::ivec3& position, const glm::ivec3& size);
};
