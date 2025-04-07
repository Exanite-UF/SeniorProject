#include "VoxelChunkData.h"

#include <chrono>
#include <glm/gtc/integer.hpp>
#include <set>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkUtility.h>
#include <stdexcept>
#include <thread>
#include <tracy/Tracy.hpp>

VoxelChunkData::VoxelChunkData(const glm::ivec3& size, bool includeMipmaps)
{
    ZoneScoped;

    setSize(size, includeMipmaps);
}

const glm::ivec3& VoxelChunkData::getSize() const
{
    return data.size;
}

void VoxelChunkData::setSize(const glm::ivec3& size)
{
    setSize(size, data.hasMipmaps);
}

void VoxelChunkData::setSize(glm::ivec3 size, bool includeMipmaps)
{
    ZoneScoped;

    // Allow for size 0
    if (size.x * size.y * size.z == 0)
    {
        this->data.size = size;
        this->data.hasMipmaps = includeMipmaps;

        data.occupancyMapIndices = { 0 };
        data.occupancyMap.resize(0);
        data.materialMap.resize(0);

        return;
    }

    // Get next power of 2
    size = {
        glm::max(2, 1 << glm::log2(size.x - 1) + 1),
        glm::max(2, 1 << glm::log2(size.y - 1) + 1),
        glm::max(2, 1 << glm::log2(size.z - 1) + 1),
    };

    auto previousSize = this->data.size;
    this->data.size = size;
    this->data.hasMipmaps = includeMipmaps;

    if (includeMipmaps)
    {
        // Store all mipmap layer data
        data.occupancyMapIndices = VoxelChunkUtility::getOccupancyMapIndices(size);
        data.occupancyMap.resize(data.occupancyMapIndices.at(data.occupancyMapIndices.size() - 1));

        // Update mipmaps if there was any previously existing data
        // If there was not, then everything is just zero
        if (previousSize != glm::ivec3(0))
        {
            updateMipmaps();
        }
    }
    else
    {
        // Only store first layer of data
        data.occupancyMapIndices = VoxelChunkUtility::getOccupancyMapIndices(size);
        data.occupancyMap.resize(data.occupancyMapIndices.at(1));
    }

    data.materialMap.resize(size.x * size.y * size.z);
}

bool VoxelChunkData::getHasMipmaps() const
{
    return data.hasMipmaps;
}

void VoxelChunkData::setHasMipmaps(bool hasMipmaps)
{
    setSize(data.size, data.hasMipmaps);
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

std::vector<uint8_t>& VoxelChunkData::getRawOccupancyMap()
{
    return data.occupancyMap;
}

std::vector<uint32_t>& VoxelChunkData::getRawOccupancyMapIndices()
{
    return data.occupancyMapIndices;
}

std::vector<uint16_t>& VoxelChunkData::getRawMaterialMap()
{
    return data.materialMap;
}

void VoxelChunkData::clearOccupancyMap()
{
    std::fill(data.occupancyMap.begin(), data.occupancyMap.end(), 0);
}

void VoxelChunkData::clearMaterialMap()
{
    std::fill(data.materialMap.begin(), data.materialMap.end(), 0);
}

void VoxelChunkData::updateMipmaps()
{
    // Todo: Remove commented code
    // // Reads a byte from the previous mipmap
    // uint getByte(ivec3 coord)
    // {
    //     int index = (coord.x + resolution.x * (coord.y + resolution.y * coord.z)) + int(previousStartByte);
    //     int bufferIndex = index / 4; // Divide by 4, because glsl does not support single byte data types, so a 4 byte data type is being used
    //     int bufferOffset = (index & 3); // Modulus 4 done using a bitmask
    //
    //     return (occupancyMap[bufferIndex] & (255 << (8 * bufferOffset))) >> (8 * bufferOffset);
    // }

    // getVoxelOccupancy();
    // // Calculate cell position and count
    // auto cellPosition = position >> 1;
    // auto cellCount = data.size >> 1;
    //
    // // Calculate byte index of cell
    // auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);
    //
    // // Calculate which bit to set
    // auto isOddPos = position & 1;
    // auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    // auto bit = 1 << bitsShifted;
    //
    // return (data.occupancyMap[cellIndex] & bit) != 0;

    // Calculate cell count
    auto cellCount = data.size >> 1;

    // Skip first layer since that's our ground truth
    for (int i = 1; i < data.occupancyMapIndices.size(); ++i)
    {
        for (int z = 0; z < cellCount.z; ++z)
        {
            for (int y = 0; y < cellCount.y; ++y)
            {
                for (int x = 0; x < cellCount.x; ++x)
                {
                    // This is the cell position in the current mipmap
                    auto currentCellPosition = glm::ivec3(x, y, z);

                    // We now need to get the 8 cells corresponding to this cell from the previous mipmap
                    uint8_t result = 0;
                    for (int bitI = 0; bitI < 8; ++bitI)
                    {
                        auto previousCellOffset = glm::ivec3((bitI & 4 >> 2), (bitI & 2 >> 1), (bitI & 2 >> 0));
                        auto previousCellPosition = currentCellPosition * 2 + previousCellOffset;

                        auto previousCellIndex = previousCellPosition.x + cellCount.x * (previousCellPosition.y + cellCount.y * previousCellPosition.z) + data.occupancyMapIndices.at(i - 1);

                        if (data.occupancyMap[previousCellIndex] != 0)
                        {
                            result |= 1 << bitI;
                        }
                    }

                    // Now set the value for the current mipmap
                    auto currentCellIndex = currentCellPosition.x + cellCount.x * (currentCellPosition.y + cellCount.y * currentCellPosition.z) + data.occupancyMapIndices.at(i);
                    data.occupancyMap[currentCellIndex] = result;
                }
            }
        }

        // Update cell count
        cellCount = cellCount >> 1;
    }
}

void VoxelChunkData::copyFrom(VoxelChunk& chunk)
{
    ZoneScoped;

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

void VoxelChunkData::copyTo(VoxelChunk& chunk)
{
    ZoneScoped;

    constexpr int sleepTime = Constants::VoxelChunk::chunkUploadSleepTimeMs;
    constexpr uint64_t uploadChunkSize = Constants::VoxelChunk::maxChunkUploadSizeBytes;

    if (chunk.getSize() != data.size)
    {
        throw std::runtime_error("Target chunk does not have the same size");
    }

    // Upload in chunks to prevent blocking the OpenGL driver
    {
        uint64_t byteCount = data.occupancyMap.size();
        int uploadChunkCount = (byteCount + uploadChunkSize - 1) / uploadChunkSize;
        for (int i = 0; i < uploadChunkCount; ++i)
        {
            ZoneScopedN("Chunked occupancy data upload");

            uint64_t remainingByteCount = byteCount - i * uploadChunkSize;
            int offset = i * uploadChunkSize;

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getOccupancyMap().bufferId);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, std::min(uploadChunkSize, remainingByteCount), reinterpret_cast<uint8_t*>(data.occupancyMap.data()) + offset);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            {
                ZoneScopedN("Sleep");

                glFlush();
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }
    }

    // Update mipmaps
    // This doesn't block the OpenGL
    chunk.updateMipMaps();

    // Upload in chunks to prevent blocking the OpenGL driver
    {
        uint64_t byteCount = data.materialMap.size() * 2;
        int uploadChunkCount = (byteCount + uploadChunkSize - 1) / uploadChunkSize;
        for (int i = 0; i < uploadChunkCount; ++i)
        {
            ZoneScopedN("Chunked material data upload");

            uint64_t remainingByteCount = byteCount - i * uploadChunkSize;
            int offset = i * uploadChunkSize;

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunk.getMaterialMap().bufferId);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, std::min(uploadChunkSize, remainingByteCount), reinterpret_cast<uint8_t*>(data.materialMap.data()) + offset);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            {
                ZoneScopedN("Sleep");

                glFlush();
                std::this_thread::sleep_for(std::chrono::milliseconds(Constants::VoxelChunk::chunkUploadSleepTimeMs));
            }
        }
    }
}

void VoxelChunkData::copyFrom(const VoxelChunkData& data)
{
    ZoneScoped;

    this->data = data.data;
}

void VoxelChunkData::copyTo(VoxelChunkData& data) const
{
    ZoneScoped;

    data.data = this->data;
}
