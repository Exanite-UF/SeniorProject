#include "VoxelChunkData.h"

#include <set>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkUtility.h>
#include <stdexcept>

const glm::ivec3& VoxelChunkData::getSize() const
{
    return size;
}

void VoxelChunkData::setSize(const glm::ivec3& size)
{
    this->size = size;

    occupancyMapIndices = VoxelChunkUtility::getOccupancyMapIndices(size);
    occupancyMap.resize(occupancyMapIndices.at(1));

    materialMap.resize(size.x * size.y * size.z);
}

bool VoxelChunkData::getVoxelOccupancy(const glm::ivec3& position) const
{
    // Calculate cell position and count
    auto cellPosition = position >> 1;
    auto cellCount = size >> 1;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto isOddPos = position & 1;
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    auto bit = 1 << bitsShifted;

    return (occupancyMap[cellIndex] & bit) != 0;
}

void VoxelChunkData::setVoxelOccupancy(const glm::ivec3& position, bool isOccupied)
{
    // Calculate cell position and count
    auto cellPosition = position >> 1;
    auto cellCount = size >> 1;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto isOddPos = position & 1;
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    auto bit = 1 << bitsShifted;

    if (isOccupied)
    {
        occupancyMap[cellIndex] |= bit;
    }
    else
    {
        occupancyMap[cellIndex] &= ~bit;
    }
}

const std::shared_ptr<Material>& VoxelChunkData::getVoxelMaterial(glm::ivec3 position) const
{
    auto& materialManager = MaterialManager::getInstance();
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);

    return materialManager.getMaterialByIndex(materialMap[voxelIndex]);
}

void VoxelChunkData::setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material)
{
    setVoxelMaterialIndex(position, material->getIndex());
}

uint16_t VoxelChunkData::getVoxelMaterialIndex(glm::ivec3 position) const
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);
    return materialMap[voxelIndex];
}

void VoxelChunkData::setVoxelMaterialIndex(const glm::ivec3& position, const uint16_t materialIndex)
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);
    materialMap[voxelIndex] = materialIndex;
}

void VoxelChunkData::clearOccupancyMap()
{
    std::fill(occupancyMap.begin(), occupancyMap.end(), 0);
}

void VoxelChunkData::clearMaterialMap()
{
    std::fill(materialMap.begin(), materialMap.end(), 0);
}

void VoxelChunkData::copyFrom(VoxelChunk& chunk)
{
    if (size != chunk.getSize())
    {
        setSize(chunk.getSize());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getOccupancyMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, occupancyMapIndices.at(1), occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getMaterialMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, materialMap.size() * 2, materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void VoxelChunkData::writeTo(VoxelChunk& chunk)
{
    if (chunk.getSize() != size)
    {
        throw std::runtime_error("Target chunk does not have the same size");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getOccupancyMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, occupancyMapIndices.at(1), occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getMaterialMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, materialMap.size() * 2, materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    chunk.updateMipMaps();
}
