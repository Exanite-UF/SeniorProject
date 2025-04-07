#include <cmath>
#include <cstdint>
#include <glm/vec3.hpp>
#include <string>

#include "VoxelChunkUtility.h"

#include <src/utilities/Assert.h>
#include <src/world/VoxelChunk.h>

std::vector<uint32_t> VoxelChunkUtility::getOccupancyMapIndices(const glm::ivec3& size)
{
    Assert::isTrue((size.x != 0) && ((size.x & (size.x - 1)) == 0), "size.x must be a power of 2");
    Assert::isTrue((size.y != 0) && ((size.y & (size.y - 1)) == 0), "size.y must be a power of 2");
    Assert::isTrue((size.z != 0) && ((size.z & (size.z - 1)) == 0), "size.z must be a power of 2");

    // The division inside the log2 call is a 4, not 2, because the mipmap generation will break if the top level mipmap has side length 1. This prevents that from occurring.
    // The mipmap generation breaks when the side length is 1 because the top level mipmap will only be represented by 1 bit instead of a full byte.
    // A minimum side length makes the top level mipmap be at least 1 byte in size.
    auto smallestSide = std::min(std::min(size.x, size.y), size.z);
    uint8_t layerCount = 1 + std::floor(std::log2(smallestSide / 4) / 2);
    layerCount = glm::min(layerCount, Constants::VoxelChunk::maxOccupancyMapLayerCount); // Limit the max number of mip maps

    std::vector<uint32_t> indices(layerCount + 1);

    // This should be the exact number of bytes that the occupancy map and all its mipmaps take up
    uint64_t totalByteCount = 0;
    for (int i = 0; i < layerCount; i++)
    {
        uint64_t divisor = (1 << (2 * i));
        divisor = divisor * divisor * divisor; // Cube the divisor
        indices[i] = totalByteCount;
        totalByteCount += size.x * size.y * size.z / 8 / divisor;
    }

    indices[indices.size() - 1] = totalByteCount;

    return indices;
}

bool VoxelChunkUtility::isValidPosition(const glm::ivec3& position, const glm::ivec3& size)
{
    if (position.x <= 0 || position.y <= 0 || position.z <= 0)
    {
        return false;
    }

    if (position.x > size.x || position.y > size.y || position.z > size.z)
    {
        return false;
    }

    return true;
}
