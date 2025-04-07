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

    auto smallestSide = std::min(std::min(size.x, size.y), size.z);
    int mipmapLayerCount = std::max(0, static_cast<int>(std::floor(std::log2(smallestSide / 2) / 2)));
    int layerCount = 1 + mipmapLayerCount;
    layerCount = glm::min(layerCount, static_cast<int>(Constants::VoxelChunk::maxOccupancyMapLayerCount)); // Limit the max number of mip maps

    std::vector<uint32_t> indices { 0 };

    // This should be the exact number of bits that the occupancy map and all its mipmaps take up
    // This won't overflow
    glm::ivec3 currentSize = size;
    uint64_t totalBitCount = 0;
    for (int i = 0; i < layerCount; i++)
    {
        totalBitCount += size.x * size.y * size.z;
        totalBitCount = ((totalBitCount + 8 - 1) / 8) * 8; // Round to next multiple of 8
        indices.push_back(totalBitCount / 8);

        currentSize >>= 1;
    }

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
