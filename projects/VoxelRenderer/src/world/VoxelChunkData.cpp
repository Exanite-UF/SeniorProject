#include "VoxelChunkData.h"

#include <set>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkUtility.h>
#include <stdexcept>

VoxelChunkData::VoxelChunkData()
{
}

VoxelChunkData::VoxelChunkData(const glm::ivec3& size)
{
    setSize(size);
}

const glm::ivec3& VoxelChunkData::getSize() const
{
    return data.size;
}

void VoxelChunkData::setSize(const glm::ivec3& size)
{
    this->data.size = size;

    data.occupancyMapIndices = VoxelChunkUtility::getOccupancyMapIndices(size);
    data.occupancyMap.resize(data.occupancyMapIndices.at(1));

    data.materialMap.resize(size.x * size.y * size.z);
}

bool VoxelChunkData::getVoxelOccupancy(const glm::ivec3& position) const
{
    // Calculate cell position and count
    auto cellPosition = position >> 1;
    auto cellCount = data.size >> 1;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto isOddPos = position & 1;
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    auto bit = 1 << bitsShifted;

    return (data.occupancyMap[cellIndex] & bit) != 0;
}

void VoxelChunkData::setVoxelOccupancy(const glm::ivec3& position, bool isOccupied)
{
    // Calculate cell position and count
    auto cellPosition = position >> 1;
    auto cellCount = data.size >> 1;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto isOddPos = position & 1;
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    auto bit = 1 << bitsShifted;

    if (isOccupied)
    {
        data.occupancyMap[cellIndex] |= bit;
    }
    else
    {
        data.occupancyMap[cellIndex] &= ~bit;
    }
}

const std::shared_ptr<Material>& VoxelChunkData::getVoxelMaterial(const glm::ivec3& position) const
{
    auto& materialManager = MaterialManager::getInstance();
    auto voxelIndex = position.x + data.size.x * (position.y + data.size.y * position.z);

    return materialManager.getMaterialByIndex(data.materialMap[voxelIndex]);
}

void VoxelChunkData::setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material)
{
    setVoxelMaterialIndex(position, material->getIndex());
}

uint16_t VoxelChunkData::getVoxelMaterialIndex(const glm::ivec3& position) const
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + data.size.x * (position.y + data.size.y * position.z);
    return data.materialMap[voxelIndex];
}

void VoxelChunkData::setVoxelMaterialIndex(const glm::ivec3& position, const uint16_t materialIndex)
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + data.size.x * (position.y + data.size.y * position.z);
    data.materialMap[voxelIndex] = materialIndex;
}


bool VoxelChunkData::isValidPosition(const glm::ivec3 position) const
{
    if (position.x <= 0 || position.y <= 0 || position.z <= 0)
    {
        return false;
    }

    if (position.x > data.size.x || position.y > data.size.y || position.z > data.size.z)
    {
        return false;
    }

    return true;
}

void VoxelChunkData::clearOccupancyMap()
{
    std::fill(data.occupancyMap.begin(), data.occupancyMap.end(), 0);
}

void VoxelChunkData::clearMaterialMap()
{
    std::fill(data.materialMap.begin(), data.materialMap.end(), 0);
}

void VoxelChunkData::copyFrom(VoxelChunk& chunk)
{
    if (data.size != chunk.getSize())
    {
        setSize(chunk.getSize());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getOccupancyMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.occupancyMapIndices.at(1), data.occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getMaterialMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.materialMap.size() * 2, data.materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void VoxelChunkData::writeTo(VoxelChunk& chunk)
{
    if (chunk.getSize() != data.size)
    {
        throw std::runtime_error("Target chunk does not have the same size");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getOccupancyMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.occupancyMapIndices.at(1), data.occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getMaterialMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.materialMap.size() * 2, data.materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    chunk.updateMipMaps();
}

void VoxelChunkData::copyFrom(VoxelChunkData& data)
{
    this->data = data.data;
}

void VoxelChunkData::writeTo(VoxelChunkData& data)
{
    data.data = this->data;
}
