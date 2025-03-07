#include <cmath>
#include <cstdint>
#include <glm/vec3.hpp>
#include <stdexcept>
#include <string>

#include "VoxelWorldUtility.h"
#include <src/world/VoxelWorld.h>

std::vector<uint32_t> VoxelWorldUtility::getOccupancyMapIndices(glm::ivec3 size)
{
    constexpr auto minSizePerAxis = Constants::VoxelWorld::minSizePerAxis;
    if (size.x < minSizePerAxis || size.y < minSizePerAxis || size.z < minSizePerAxis)
    {
        throw std::runtime_error("The minimum size of a voxel world along an axis is " + std::to_string(minSizePerAxis));
    }

    uint8_t layerCount = 1 + std::floor(std::log2(std::min(std::min(size.x, size.y), size.z) / 4 /*This is a 4 and not a 2, because the mip map generation will break if the top level mip map has side length 1. This prevents that from occuring.*/) / 2); // This is what the name says it is
    layerCount = glm::min(layerCount, Constants::VoxelWorld::maxOccupancyMapLayerCount); // Limit the max number of mip maps

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

std::array<GLuint, Constants::VoxelWorld::paletteMapLayerCount + 1> VoxelWorldUtility::getPaletteMapIndices(glm::ivec3 size)
{
    std::array<GLuint, Constants::VoxelWorld::paletteMapLayerCount + 1> indices;

    // This should be the exact number of bytes that the palette map and all its mipmaps take up
    std::uint64_t totalByteCount = 0;
    for (int i = 0; i < 3; i++)
    {
        std::uint64_t divisor = (1 << (2 * i));
        divisor = divisor * divisor * divisor; // Cube the divisor
        indices[i] = totalByteCount;
        totalByteCount += 4 * size.x * size.y * size.z / 8 / divisor;
    }

    indices[indices.size() - 1] = totalByteCount;

    return indices;
}
