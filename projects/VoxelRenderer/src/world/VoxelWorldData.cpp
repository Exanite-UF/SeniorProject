#include "VoxelWorldData.h"

#include "MaterialManager.h"

#include <src/world/VoxelWorldUtility.h>
#include <stdexcept>

void VoxelWorldData::copyFrom(VoxelWorld& world)
{
    if (size != world.getSize())
    {
        setSize(world.getSize());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world.getOccupancyMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, occupancyMapIndices.at(1), occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world.getMaterialMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, materialMapIndices.at(materialMapIndices.size() - 1), materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void VoxelWorldData::writeTo(VoxelWorld& world)
{
    if (world.getSize() != size)
    {
        throw std::runtime_error("Target world does not have the same size");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world.getOccupancyMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, occupancyMapIndices.at(1), occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world.getMaterialMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, materialMapIndices.at(materialMapIndices.size() - 1), materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    world.updateMipMaps();
}

void VoxelWorldData::clearOccupancy()
{
    std::fill(occupancyMap.begin(), occupancyMap.end(), 0);
}

const glm::ivec3& VoxelWorldData::getSize()
{
    return size;
}

void VoxelWorldData::setSize(glm::ivec3 size)
{
    this->size = size;

    occupancyMapIndices = VoxelWorldUtility::getOccupancyMapIndices(size);
    occupancyMap.resize(occupancyMapIndices.at(1));

    materialMapIndices = VoxelWorldUtility::getMaterialMapIndices(size);
    materialMap.resize(materialMapIndices.at(materialMapIndices.size() - 1));

    flattenedMaterialMap.resize(size.x * size.y * size.z);
}

void VoxelWorldData::setVoxelOccupancy(glm::ivec3 position, bool isOccupied)
{
    // Calculate cell position and count
    auto cellPosition = position / 2;
    auto cellCount = size / 2;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto oddX = position.x % 2;
    auto oddY = position.y % 2;
    auto oddZ = position.z % 2;
    auto bit = 1 << (1 * oddX + 2 * oddY + 4 * oddZ);

    if (isOccupied)
    {
        occupancyMap[cellIndex] |= bit;
    }
    else
    {
        occupancyMap[cellIndex] &= ~bit;
    }
}

void VoxelWorldData::setVoxelMaterial(glm::ivec3 position, const Material& material)
{
    setVoxelMaterial(position, material.getIndex());
}

void VoxelWorldData::setVoxelMaterial(glm::ivec3 position, const uint16_t material)
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);
    flattenedMaterialMap[voxelIndex] = material;
}

void VoxelWorldData::setVoxelMipMappedMaterial(glm::ivec3 position, uint8_t material0, uint8_t material1, uint8_t material2)
{
    // Each mipmapped material ID is 12 bits, separated into 4 bit parts
    // material0 is the ID stored in the lowest/highest resolution mipmap layer
    std::array materialIdParts = { material0, material1, material2 };

    auto cellCount = size;
    auto cellPosition = glm::ivec3(position.x, position.y, position.z);
    for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::materialMapLayerCount; ++mipMapI)
    {
        // Calculate cell position and count
        cellCount /= 4;
        cellPosition /= 4;

        // Calculate uint index of cell
        auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);
        cellIndex += materialMapIndices.at(mipMapI) / 4;

        // Calculate which set of 4-bits to set
        auto oddX = (position.x >> (2 * mipMapI)) % 2;
        auto oddY = (position.y >> (2 * mipMapI)) % 2;
        auto oddZ = (position.z >> (2 * mipMapI)) % 2;

        auto cellValue = materialIdParts[mipMapI];

        auto bitmask = 0b1111;
        auto bitsShifted = 4 * (1 * oddX + 2 * oddY + 4 * oddZ);

        reinterpret_cast<uint32_t*>(materialMap.data())[cellIndex] &= ~(0b1111 < bitsShifted);
        reinterpret_cast<uint32_t*>(materialMap.data())[cellIndex] |= cellValue < bitsShifted;
    }
}

void VoxelWorldData::decodeMaterialMipMap()
{
    // TODO: Consider optimizing this. This currently runs in (w^3 * 3) time. Can improve to (w^3 + w^3 / 4^3 + w^3 / 4^6) time by iterating each mipmap once.
    // Realistically, this function is called at most once per voxel world, if at all.
    auto& materialManager = MaterialManager::getInstance();
    for (int z = 0; z < size.x; ++z)
    {
        for (int y = 0; y < size.x; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                uint16_t materialMipMappedId = 0;

                auto cellCount = size;
                auto cellPosition = glm::ivec3(x, y, z);
                for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::materialMapLayerCount; ++mipMapI)
                {
                    // Calculate cell position and count
                    cellCount /= 4;
                    cellPosition /= 4;

                    // Calculate uint index of cell
                    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);
                    cellIndex += materialMapIndices.at(mipMapI) / 4;

                    // Calculate which set of 4-bits to get
                    auto oddX = (x >> (2 * mipMapI)) % 2;
                    auto oddY = (y >> (2 * mipMapI)) % 2;
                    auto oddZ = (z >> (2 * mipMapI)) % 2;

                    auto cellValue = reinterpret_cast<uint32_t*>(materialMap.data())[cellIndex];
                    auto bitsShifted = 4 * (1 * oddX + 2 * oddY + 4 * oddZ);
                    auto partialId = cellValue & (0b1111 << bitsShifted);
                    materialMipMappedId |= (partialId >> bitsShifted) << (mipMapI * 4);
                }

                // setVoxelMaterial(glm::ivec3(x, y, z), materialMipMappedId); // For debugging. This lets you see the mipMappedId.
                setVoxelMaterial(glm::ivec3(x, y, z), materialManager.getMaterialIndexByMipMappedId(materialMipMappedId));
            }
        }
    }
}

void VoxelWorldData::encodeMaterialMipMap()
{
    for (int z = 0; z < size.x; ++z)
    {
        for (int y = 0; y < size.x; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                // TODO: This isn't meant to fully work. To properly encode the material mipmaps, we need a solver!
                auto materialIndexI = x + size.x * (y + size.y * z);
                auto materialIndex = flattenedMaterialMap[materialIndexI];

                auto material0 = materialIndex & (0b1111 << 0) >> 0;
                auto material1 = materialIndex & (0b1111 << 4) >> 4;
                auto material2 = materialIndex & (0b1111 << 8) >> 8;

                setVoxelMipMappedMaterial(glm::ivec3(x, y, z), material0, material1, material2);
            }
        }
    }
}
