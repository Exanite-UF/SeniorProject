#include "VoxelWorldData.h"

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
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, materialIdMapIndices.at(materialIdMapIndices.size() - 1), materialMap.data());
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
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, materialIdMapIndices.at(materialIdMapIndices.size() - 1), materialMap.data());
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

    materialIdMapIndices = VoxelWorldUtility::getMaterialMapIndices(size);
    materialMap.resize(materialIdMapIndices.at(materialIdMapIndices.size() - 1));

    flattenedMaterialMap.resize(size.x * size.y * size.z);
}

void VoxelWorldData::setVoxelOccupancy(glm::ivec3 position, bool isOccupied)
{
    auto cellPosition = position / 2;
    auto halfSize = size / 2;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + halfSize.x * (cellPosition.y + halfSize.y * cellPosition.z);

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

    // TODO
}

void VoxelWorldData::decodeMaterialMipMap()
{
    // TODO
}

void VoxelWorldData::encodeMaterialMipMap()
{
    // TODO
}
