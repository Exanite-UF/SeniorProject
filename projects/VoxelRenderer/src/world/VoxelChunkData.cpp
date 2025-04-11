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
    setSize(data.size, hasMipmaps);
}

int VoxelChunkData::getOccupancyMipmapCount() const
{
    return std::max(0, static_cast<int>(data.occupancyMapIndices.size()) - 2);
}

int VoxelChunkData::getOccupancyLayerCount() const
{
    return std::max(0, static_cast<int>(data.occupancyMapIndices.size()) - 1);
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

    return (data.occupancyMap.at(cellIndex) & bit) != 0;
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
        data.occupancyMap.at(cellIndex) |= bit;
    }
    else
    {
        data.occupancyMap.at(cellIndex) &= ~bit;
    }
}

bool VoxelChunkData::getMipmapVoxelOccupancy(const glm::ivec3& positionInLevel, const int level) const
{
    // Calculate cell position and count
    auto cellPosition = positionInLevel >> 1;
    auto cellCount = data.size >> (2 * level + 1);

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + data.occupancyMapIndices.at(level);

    // Calculate which bit to set
    auto isOddPos = positionInLevel & 1;
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    auto bit = 1 << bitsShifted;

    return (data.occupancyMap.at(cellIndex) & bit) != 0;
}

void VoxelChunkData::setMipmapVoxelOccupancy(const glm::ivec3& positionInLevel, int level, bool isOccupied)
{
    // Calculate cell position and count
    auto cellPosition = positionInLevel >> 1;
    auto cellCount = data.size >> (2 * level + 1);

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + data.occupancyMapIndices.at(level);

    // Calculate which bit to set
    auto isOddPos = positionInLevel & 1;
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
    auto bit = 1 << bitsShifted;

    if (isOccupied)
    {
        data.occupancyMap.at(cellIndex) |= bit;
    }
    else
    {
        data.occupancyMap.at(cellIndex) &= ~bit;
    }
}

const std::shared_ptr<Material>& VoxelChunkData::getVoxelMaterial(const glm::ivec3& position) const
{
    auto& materialManager = MaterialManager::getInstance();
    auto voxelIndex = position.x + data.size.x * (position.y + data.size.y * position.z);

    return materialManager.getMaterialByIndex(data.materialMap.at(voxelIndex));
}

void VoxelChunkData::setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material)
{
    setVoxelMaterialIndex(position, material->getIndex());
}

uint16_t VoxelChunkData::getVoxelMaterialIndex(const glm::ivec3& position) const
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + data.size.x * (position.y + data.size.y * position.z);
    return data.materialMap.at(voxelIndex);
}

void VoxelChunkData::setVoxelMaterialIndex(const glm::ivec3& position, const uint16_t materialIndex)
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + data.size.x * (position.y + data.size.y * position.z);
    data.materialMap.at(voxelIndex) = materialIndex;
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
    ZoneScoped;

    std::fill(data.occupancyMap.begin(), data.occupancyMap.end(), 0);
}

void VoxelChunkData::clearMaterialMap()
{
    ZoneScoped;

    std::fill(data.materialMap.begin(), data.materialMap.end(), 0);
}

void VoxelChunkData::updateMipmaps()
{
    ZoneScoped;

    // Skip first layer since that's the ground truth
    for (int i = 1; i < data.occupancyMapIndices.size() - 1; ++i)
    {
        // Calculate cell count
        auto previousCellCount = data.size >> (2 * (i - 1) + 1);
        auto currentCellCount = data.size >> (2 * i + 1);

        for (int currentCellZ = 0; currentCellZ < currentCellCount.z; ++currentCellZ)
        {
            for (int currentCellY = 0; currentCellY < currentCellCount.y; ++currentCellY)
            {
                for (int currentCellX = 0; currentCellX < currentCellCount.x; ++currentCellX)
                {
                    // This is the cell position in the current mipmap
                    auto currentCellPosition = glm::ivec3(currentCellX, currentCellY, currentCellZ);

                    // 64 bits in the previous mipmap corresponds to 1 bit in the current mipmap
                    // 64 bits corresponds to a 4x4x4 voxel region or a 2x2x2 cell region
                    //
                    // However, we need to get a full byte,
                    // so we actually need to loop over a 4x4x4 cell region in the previous mipmap to get a cell for this mipmap
                    //
                    // This requires the use of two sets of 3D for loops
                    uint8_t result = 0;
                    uint8_t bit = 1;
                    for (int currentVoxelZ = 0; currentVoxelZ < 2; ++currentVoxelZ)
                    {
                        for (int currentVoxelY = 0; currentVoxelY < 2; ++currentVoxelY)
                        {
                            for (int currentVoxelX = 0; currentVoxelX < 2; ++currentVoxelX)
                            {
                                bool isOccupied = false;

                                for (int previousCellZ = 0; !isOccupied && previousCellZ < 2; ++previousCellZ)
                                {
                                    for (int previousCellY = 0; !isOccupied && previousCellY < 2; ++previousCellY)
                                    {
                                        for (int previousCellX = 0; !isOccupied && previousCellX < 2; ++previousCellX)
                                        {
                                            auto previousCellOffset = glm::ivec3(previousCellX, previousCellY, previousCellZ);
                                            auto previousCellPosition = currentCellPosition * 2 + previousCellOffset;
                                            auto previousCellIndex = previousCellPosition.x + previousCellCount.x * (previousCellPosition.y + previousCellCount.y * previousCellPosition.z) + data.occupancyMapIndices.at(i - 1);
                                            Assert::isTrue(previousCellIndex >= data.occupancyMapIndices.at(i - 1) && previousCellIndex < data.occupancyMapIndices.at(i), "previousCellIndex is out of bounds");

                                            isOccupied = data.occupancyMap.at(previousCellIndex) != 0;
                                        }
                                    }
                                }

                                if (isOccupied)
                                {
                                    result |= bit;
                                }

                                bit <<= 1;
                            }
                        }
                    }

                    // Now set the value for the current mipmap
                    auto currentCellIndex = currentCellPosition.x + currentCellCount.x * (currentCellPosition.y + currentCellCount.y * currentCellPosition.z) + data.occupancyMapIndices.at(i);
                    Assert::isTrue(currentCellIndex >= data.occupancyMapIndices.at(i) && currentCellIndex < data.occupancyMapIndices.at(i + 1), "currentCellIndex is out of bounds");
                    data.occupancyMap.at(currentCellIndex) = result;
                }
            }
        }
    }
}

void VoxelChunkData::copyFrom(VoxelChunk& other)
{
    ZoneScoped;

    if (data.size != other.getSize())
    {
        setSize(other.getSize());
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, other.getOccupancyMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.occupancyMapIndices.at(1), data.occupancyMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, other.getMaterialMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, data.materialMap.size() * 2, data.materialMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void VoxelChunkData::copyTo(VoxelChunk& other) const
{
    ZoneScoped;

    constexpr int sleepTime = Constants::VoxelChunk::chunkUploadSleepTimeMs;
    constexpr uint64_t uploadChunkSize = Constants::VoxelChunk::maxChunkUploadSizeBytes;

    if (other.getSize() != data.size)
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

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, other.getOccupancyMap().bufferId);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, std::min(uploadChunkSize, remainingByteCount), const_cast<uint8_t*>(data.occupancyMap.data()) + offset);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            {
                ZoneScopedN("Sleep");

                glFlush();
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }
    }

    // Update mipmaps
    // This doesn't block the OpenGL driver
    other.updateMipMaps();

    // Upload in chunks to prevent blocking the OpenGL driver
    {
        uint64_t byteCount = data.materialMap.size() * 2;
        int uploadChunkCount = (byteCount + uploadChunkSize - 1) / uploadChunkSize;
        for (int i = 0; i < uploadChunkCount; ++i)
        {
            ZoneScopedN("Chunked material data upload");

            uint64_t remainingByteCount = byteCount - i * uploadChunkSize;
            int offset = i * uploadChunkSize;

            glBindBuffer(GL_SHADER_STORAGE_BUFFER, other.getMaterialMap().bufferId);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, std::min(uploadChunkSize, remainingByteCount), reinterpret_cast<const uint8_t*>(data.materialMap.data()) + offset);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            {
                ZoneScopedN("Sleep");

                glFlush();
                std::this_thread::sleep_for(std::chrono::milliseconds(Constants::VoxelChunk::chunkUploadSleepTimeMs));
            }
        }
    }
}

void VoxelChunkData::copyFrom(const VoxelChunkData& other)
{
    ZoneScoped;

    data = other.data;
}

void VoxelChunkData::copyTo(VoxelChunkData& other) const
{
    ZoneScoped;

    other.data = data;
}

void VoxelChunkData::copyToLod(VoxelChunkData& lod) const
{
    ZoneScoped;

    // Update lod's size
    lod.setSize(data.size >> 1);

    // Generate occupancy LOD
    {
        ZoneScopedN("Generate occupancy LOD");

        // Calculate cell count
        auto selfCellCount = data.size >> 1;
        auto lodCellCount = data.size >> 2;

        for (int lodZ = 0; lodZ < lodCellCount.z; ++lodZ)
        {
            for (int lodY = 0; lodY < lodCellCount.y; ++lodY)
            {
                for (int lodX = 0; lodX < lodCellCount.x; ++lodX)
                {
                    // This is the cell position in the LOD
                    auto lodCellPosition = glm::ivec3(lodX, lodY, lodZ);

                    // We now need to get the 8 cells corresponding to this cell from the original data
                    uint8_t result = 0;
                    for (int bitI = 0; bitI < 8; ++bitI)
                    {
                        auto selfCellOffset = glm::ivec3(((bitI & 0b001) >> 0), ((bitI & 0b010) >> 1), ((bitI & 0b100) >> 2));
                        auto selfCellPosition = lodCellPosition * 2 + selfCellOffset;
                        auto selfCellIndex = selfCellPosition.x + selfCellCount.x * (selfCellPosition.y + selfCellCount.y * selfCellPosition.z);
                        Assert::isTrue(selfCellIndex >= data.occupancyMapIndices.at(0) && selfCellIndex < data.occupancyMapIndices.at(1), "selfCellIndex is out of bounds");

                        if (data.occupancyMap.at(selfCellIndex) != 0)
                        {
                            result |= 1 << bitI;
                        }
                    }

                    // Now set the value in the LOD
                    auto lodCellIndex = lodCellPosition.x + lodCellCount.x * (lodCellPosition.y + lodCellCount.y * lodCellPosition.z);
                    Assert::isTrue(lodCellIndex >= lod.data.occupancyMapIndices.at(0) && lodCellIndex < lod.data.occupancyMapIndices.at(1), "lodCellIndex is out of bounds");
                    lod.data.occupancyMap.at(lodCellIndex) = result;
                }
            }
        }
    }

    // Generate material LOD
    {
        ZoneScopedN("Generate material LOD");

        auto lodSize = data.size >> 1;

        for (int lodZ = 0; lodZ < lodSize.z; ++lodZ)
        {
            for (int lodY = 0; lodY < lodSize.y; ++lodY)
            {
                for (int lodX = 0; lodX < lodSize.x; ++lodX)
                {
                    // This is the voxel position in the LOD
                    auto lodPosition = glm::ivec3(lodX, lodY, lodZ);
                    if (!lod.getVoxelOccupancy(lodPosition))
                    {
                        // Skip air
                        continue;
                    }

                    // We now need to get 1 of the 8 original materials to show in the LOD
                    // We will prefer materials from air-exposed voxels to make sure materials that are not exposed are not exposed in the LOD
                    uint8_t airMask = 0; // Tracks voxels that are invalid for material selection. Note that at least 1 voxel is always valid due to how LOD occupancy is generated.
                    uint8_t airExposedMask = 0; // Tracks voxels that are preferred for material selection
                    for (int z = 0; z < 2; ++z)
                    {
                        for (int y = 0; y < 2; ++y)
                        {
                            for (int x = 0; x < 2; ++x)
                            {
                                auto selfPosition = lodPosition * 2 + glm::ivec3(x, y, z);

                                // Check if voxel itself is air
                                if (!getVoxelOccupancy(selfPosition))
                                {
                                    airMask |= 1 << ((z << 2) | (y << 1) | (x << 0));

                                    continue;
                                }

                                // Check if voxel is air exposed
                                // We only check the 3 outer directions
                                // If the voxel checked is outside the chunk, then it is treated as non-air
                                auto outerPositionX = selfPosition + glm::ivec3(x * 2 - 1, 0, 0);
                                bool isAirX = VoxelChunkUtility::isValidPosition(outerPositionX, data.size) && !getVoxelOccupancy(outerPositionX);

                                auto outerPositionY = selfPosition + glm::ivec3(0, y * 2 - 1, 0);
                                bool isAirY = VoxelChunkUtility::isValidPosition(outerPositionY, data.size) && !getVoxelOccupancy(outerPositionY);

                                auto outerPositionZ = selfPosition + glm::ivec3(0, 0, z * 2 - 1);
                                bool isAirZ = VoxelChunkUtility::isValidPosition(outerPositionZ, data.size) && !getVoxelOccupancy(outerPositionZ);

                                if (isAirX || isAirY || isAirZ)
                                {
                                    airExposedMask |= 1 << ((z << 2) | (y << 1) | (x << 0));
                                }
                            }
                        }
                    }

                    // If no voxels are air exposed, then use the inverse of the air mask
                    // The inverse of the air mask refers to any voxel that is occupied
                    uint8_t validMask = airExposedMask == 0 ? ~airMask : airExposedMask;
                    Assert::isTrue(validMask != 0, "validMask should never be zero. This means this algorithm was implemented incorrectly");

                    // This will determine which of the valid materials we will take
                    // Ideally, we want the valid material selected to be uniformly selected and deterministic
                    // The selection process also needs to be fast
                    uint8_t index = ((lodZ & 1) << 2) | ((lodY & 1) << 1) | ((lodX & 1) << 0);

                    uint8_t currentBitI = 0;
                    uint8_t matchedCount = 0;
                    while (true)
                    {
                        if ((validMask & (1 << currentBitI)) != 0)
                        {
                            matchedCount++;
                        }

                        if (matchedCount > index)
                        {
                            // We found our material
                            // Set it as the result and exit

                            auto voxelPosition = lodPosition * 2 + glm::ivec3(((currentBitI & 0b001) >> 0), ((currentBitI & 0b010) >> 1), ((currentBitI & 0b100) >> 2));
                            auto materialIndex = getVoxelMaterialIndex(voxelPosition);
                            lod.setVoxelMaterialIndex(lodPosition, materialIndex);

                            break;
                        }

                        currentBitI = (currentBitI + 1) % 8;
                    }
                }
            }
        }
    }
}
