#include "VoxelWorldData.h"

#include <src/utilities/Assert.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelWorldUtility.h>
#include <stdexcept>

const glm::ivec3& VoxelWorldData::getSize() const
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

bool VoxelWorldData::getVoxelOccupancy(glm::ivec3 position) const
{
    // Calculate cell position and count
    auto cellPosition = position >> 1;
    auto cellCount = size >> 1;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto isOddPos = position & 1;
    auto bitsShifted = (isOddPos.x << 0) | (isOddPos.y << 1) | (isOddPos.z << 2);
    auto bit = 1 << bitsShifted;

    return occupancyMap[cellIndex] & bit != 0;
}

void VoxelWorldData::setVoxelOccupancy(glm::ivec3 position, bool isOccupied)
{
    // Calculate cell position and count
    auto cellPosition = position >> 1;
    auto cellCount = size >> 1;

    // Calculate byte index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);

    // Calculate which bit to set
    auto isOddPos = position & 1;
    auto bitsShifted = (isOddPos.x << 0) | (isOddPos.y << 1) | (isOddPos.z << 2);
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

const std::shared_ptr<Material>& VoxelWorldData::getVoxelMaterial(glm::ivec3 position) const
{
    auto& materialManager = MaterialManager::getInstance();
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);

    return materialManager.getMaterialByIndex(flattenedMaterialMap[voxelIndex]);
}

void VoxelWorldData::setVoxelMaterial(glm::ivec3 position, const std::shared_ptr<Material>& material)
{
    setVoxelMaterial(position, material->getIndex());
}

void VoxelWorldData::setVoxelMaterial(glm::ivec3 position, const uint16_t materialIndex)
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);
    flattenedMaterialMap[voxelIndex] = materialIndex;
}

uint8_t VoxelWorldData::getVoxelPartialPaletteId(glm::ivec3 position, int level) const
{
    // Voxels correspond to 1x1x1 regions
    // This won't always be the position of the original voxel because of mipmapping
    // Each mipmap level increases region size by 4 times
    auto voxelPosition = position >> (level << 1);

    // Cells correspond to 2x2x2 regions
    auto cellCount = size >> ((level << 1) + 1);
    auto cellPosition = position >> ((level << 1) + 1);

    // Calculate uint index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + (materialMapIndices.at(level) >> 2);

    // Calculate which set of 4-bits to get
    auto oddX = voxelPosition.x & 1;
    auto oddY = voxelPosition.y & 1;
    auto oddZ = voxelPosition.z & 1;
    auto bitsShifted = ((oddX << 0) | (oddY << 1) | (oddZ << 2)) << 2;

    // Get value from cell and extract the 4 bit segment that we want
    auto cellValue = reinterpret_cast<const uint32_t*>(materialMap.data())[cellIndex];
    auto voxelValue = (cellValue & (0b1111 << bitsShifted)) >> bitsShifted;

    return voxelValue;
}

void VoxelWorldData::setVoxelPartialPaletteId(glm::ivec3 position, uint8_t partialPaletteId, int level)
{
    // Voxels correspond to 1x1x1 regions
    // This won't always be the position of the original voxel because of mipmapping
    // Each mipmap level increases region size by 4 times
    auto voxelPosition = position >> (level << 1);

    // Cells correspond to 2x2x2 regions
    auto cellCount = size >> ((level << 1) + 1);
    auto cellPosition = position >> ((level << 1) + 1);

    // Calculate uint index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + (materialMapIndices.at(level) >> 2);

    // Calculate which set of 4-bits to set
    auto oddX = voxelPosition.x & 1;
    auto oddY = voxelPosition.y & 1;
    auto oddZ = voxelPosition.z & 1;
    auto bitsShifted = ((oddX << 0) | (oddY << 1) | (oddZ << 2)) << 2;

    // Get value that should be stored for this voxel
    auto voxelValue = partialPaletteId;

    // Set data
    auto cellData = reinterpret_cast<uint32_t*>(materialMap.data());
    cellData[cellIndex] &= ~(0b1111 << bitsShifted);
    cellData[cellIndex] |= voxelValue << bitsShifted;
}

uint16_t VoxelWorldData::getVoxelPaletteId(glm::ivec3 position) const
{
    // Each mipmapped material ID is 12 bits, separated into 4 bit parts
    // The least significant 4 bits represents the ID stored in the lowest/highest resolution mipmap layer
    uint16_t result = 0;

    // Set for each mipmap level
    for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::materialMapLayerCount; ++mipMapI)
    {
        auto partialId = getVoxelPartialPaletteId(position, mipMapI);

        // Store partial ID as part of result
        result |= partialId << (mipMapI * 4);
    }

    return result;
}

void VoxelWorldData::setVoxelPaletteId(glm::ivec3 position, uint8_t palette0, uint8_t palette1, uint8_t palette2)
{
    // Each mipmapped material ID is 12 bits, separated into 4 bit parts
    // material0 is the ID stored in the lowest/highest resolution mipmap layer
    std::array materialIdParts = { palette0, palette1, palette2 };

    // Set for each mipmap level
    for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::materialMapLayerCount; ++mipMapI)
    {
        setVoxelPartialPaletteId(position, materialIdParts[mipMapI], mipMapI);
    }
}

void VoxelWorldData::decodeMaterialMipMap()
{
    MeasureElapsedTimeScope scope("VoxelWorldData::decodeMaterialMipMap");

    // See setVoxelMipMappedMaterial for comments on how this works. The code is very similar.

    // TODO: Consider optimizing this. This currently runs in (w^3 * 3) time. Can improve to (w^3 + w^3 / 4^3 + w^3 / 4^6) time by iterating each mipmap once.
    // Realistically, this function is called at most once per voxel world, if at all.
    auto& materialManager = MaterialManager::getInstance();
    for (int z = 0; z < size.x; ++z)
    {
        for (int y = 0; y < size.x; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                uint16_t materialMippedId = getVoxelPaletteId(glm::ivec3(x, y, z));
                uint32_t materialIndex = materialManager.getMaterialIndexByPaletteId(materialMippedId);

                // setVoxelMaterial(glm::ivec3(x, y, z), materialMipMappedId); // For debugging. This lets you see the mipMappedId.
                setVoxelMaterial(glm::ivec3(x, y, z), materialIndex);
            }
        }
    }
}

void VoxelWorldData::encodeMaterialMipMap()
{
    MeasureElapsedTimeScope scope("VoxelWorldData::encodeMaterialMipMap");
    for (int z = 0; z < size.x; ++z)
    {
        for (int y = 0; y < size.x; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                // TODO: This isn't meant to fully work. To properly encode the material mipmaps, we need a solver!
                auto materialIndexI = x + size.x * (y + size.y * z);
                auto materialIndex = flattenedMaterialMap[materialIndexI];

                auto material0 = (materialIndex & (0b1111 << 0)) >> 0;
                auto material1 = (materialIndex & (0b1111 << 4)) >> 4;
                auto material2 = (materialIndex & (0b1111 << 8)) >> 8;

                setVoxelPaletteId(glm::ivec3(x, y, z), material0, material1, material2);
            }
        }
    }
}

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

void VoxelWorldData::clearMaterials()
{
    std::fill(flattenedMaterialMap.begin(), flattenedMaterialMap.end(), 0);
}

void VoxelWorldData::clearMaterialMipMap()
{
    std::fill(materialMap.begin(), materialMap.end(), 0);
}
