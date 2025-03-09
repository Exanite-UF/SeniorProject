#include "VoxelWorldData.h"

#include <set>
#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelWorldUtility.h>
#include <stdexcept>

const glm::ivec3& VoxelWorldData::getSize() const
{
    return size;
}

void VoxelWorldData::setSize(const glm::ivec3& size)
{
    this->size = size;

    occupancyMapIndices = VoxelWorldUtility::getOccupancyMapIndices(size);
    occupancyMap.resize(occupancyMapIndices.at(1));

    paletteMapIndices = VoxelWorldUtility::getPaletteMapIndices(size);
    paletteMap.resize(paletteMapIndices.at(paletteMapIndices.size() - 1));

    materialMap.resize(size.x * size.y * size.z);
}

bool VoxelWorldData::getVoxelOccupancy(const glm::ivec3& position) const
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

    return (occupancyMap[cellIndex] & bit) != 0;
}

void VoxelWorldData::setVoxelOccupancy(const glm::ivec3& position, bool isOccupied)
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

    return materialManager.getMaterialByIndex(materialMap[voxelIndex]);
}

void VoxelWorldData::setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material)
{
    setVoxelMaterial(position, material->getIndex());
}

void VoxelWorldData::setVoxelMaterial(const glm::ivec3& position, const uint16_t materialIndex)
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);
    materialMap[voxelIndex] = materialIndex;
}

uint8_t VoxelWorldData::getVoxelPartialPaletteId(const glm::ivec3& position, int level) const
{
    // Voxels correspond to 1x1x1 regions
    // This won't always be the position of the original voxel because of mipmapping
    // Each mipmap level increases region size by 4 times
    auto voxelPosition = position >> (level << 1);

    // Cells correspond to 2x2x2 regions
    auto cellCount = size >> ((level << 1) + 1);
    auto cellPosition = position >> ((level << 1) + 1);

    // Calculate uint index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + (paletteMapIndices.at(level) >> 2);

    // Calculate which set of 4-bits to get
    auto oddX = voxelPosition.x & 1;
    auto oddY = voxelPosition.y & 1;
    auto oddZ = voxelPosition.z & 1;
    auto bitsShifted = ((oddX << 0) | (oddY << 1) | (oddZ << 2)) << 2;

    // Get value from cell and extract the 4 bit segment that we want
    auto cellValue = reinterpret_cast<const uint32_t*>(paletteMap.data())[cellIndex];
    auto voxelValue = (cellValue & (0b1111 << bitsShifted)) >> bitsShifted;

    return voxelValue;
}

void VoxelWorldData::setVoxelPartialPaletteId(const glm::ivec3& position, uint8_t partialPaletteId, int level)
{
    // Voxels correspond to 1x1x1 regions
    // This won't always be the position of the original voxel because of mipmapping
    // Each mipmap level increases region size by 4 times
    auto voxelPosition = position >> (level << 1);

    // Cells correspond to 2x2x2 regions
    auto cellCount = size >> ((level << 1) + 1);
    auto cellPosition = position >> ((level << 1) + 1);

    // Calculate uint index of cell
    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + (paletteMapIndices.at(level) >> 2);

    // Calculate which set of 4-bits to set
    auto oddX = voxelPosition.x & 1;
    auto oddY = voxelPosition.y & 1;
    auto oddZ = voxelPosition.z & 1;
    auto bitsShifted = ((oddX << 0) | (oddY << 1) | (oddZ << 2)) << 2;

    // Get value that should be stored for this voxel
    auto voxelValue = partialPaletteId;

    // Set data
    auto cellData = reinterpret_cast<uint32_t*>(paletteMap.data());
    cellData[cellIndex] &= ~(0b1111 << bitsShifted);
    cellData[cellIndex] |= voxelValue << bitsShifted;
}

uint16_t VoxelWorldData::getVoxelPaletteId(const glm::ivec3& position) const
{
    // Each mipmapped material ID is 12 bits, separated into 4 bit parts
    // The least significant 4 bits represents the ID stored in the lowest/highest resolution mipmap layer
    uint16_t result = 0;

    // Set for each mipmap level
    for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::paletteMapLayerCount; ++mipMapI)
    {
        auto partialId = getVoxelPartialPaletteId(position, mipMapI);

        // Store partial ID as part of result
        result |= partialId << (mipMapI * 4);
    }

    return result;
}

void VoxelWorldData::setVoxelPaletteId(const glm::ivec3& position, uint16_t paletteId)
{
    auto palette0 = (paletteId & (0b1111 << 0)) >> 0;
    auto palette1 = (paletteId & (0b1111 << 4)) >> 4;
    auto palette2 = (paletteId & (0b1111 << 8)) >> 8;

    return setVoxelPaletteId(position, palette0, palette1, palette2);
}

void VoxelWorldData::setVoxelPaletteId(const glm::ivec3& position, uint8_t palette0, uint8_t palette1, uint8_t palette2)
{
    // Each mipmapped material ID is 12 bits, separated into 4 bit parts
    // material0 is the ID stored in the lowest/highest resolution mipmap layer
    std::array materialIdParts = { palette0, palette1, palette2 };

    // Set for each mipmap level
    for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::paletteMapLayerCount; ++mipMapI)
    {
        setVoxelPartialPaletteId(position, materialIdParts[mipMapI], mipMapI);
    }
}

void VoxelWorldData::decodePaletteMap()
{
    MeasureElapsedTimeScope scope("VoxelWorldData::decodeMaterialMipMap");

    auto& materialManager = MaterialManager::getInstance();
    for (int z = 0; z < size.x; ++z)
    {
        for (int y = 0; y < size.x; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                auto isOccupied = getVoxelOccupancy(glm::ivec3(x, y, z));
                if (!isOccupied)
                {
                    // Ignore unoccupied voxels
                    continue;
                }

                uint16_t paletteId = getVoxelPaletteId(glm::ivec3(x, y, z));
                uint32_t materialIndex = materialManager.getMaterialIndexByPaletteId(paletteId);

                setVoxelMaterial(glm::ivec3(x, y, z), materialIndex);
            }
        }
    }
}

void VoxelWorldData::encodePaletteMap()
{
    MeasureElapsedTimeScope scope("VoxelWorldData::encodeMaterialMipMap");

    // TODO: Refactor. This currently hardcodes the entire encoding process

    auto& materialManager = MaterialManager::getInstance();
    auto& palettes = materialManager.palettes;

    // TODO: For debugging. Don't reset the material palettes
    // Reset material palettes
    palettes->clear();
    palettes->materialIndices.emplace(0);
    palettes->children[0]->materialIndices.emplace(0);
    palettes->children[0]->children[0]->materialIndices.emplace(0);
    palettes->children[0]->children[0]->children[0]->materialIndices.emplace(0);

    materialManager.materialIndexByPaletteId.fill(0);

    // TODO: For debugging.
    std::set<uint32_t> uniqueIndices {};

    for (int z = 0; z < size.x; ++z)
    {
        for (int y = 0; y < size.x; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                auto isOccupied = getVoxelOccupancy(glm::ivec3(x, y, z));
                if (!isOccupied)
                {
                    // Ignore unoccupied voxels
                    continue;
                }

                // TODO: For debugging
                uniqueIndices.emplace(materialMap[x + size.x * (y + size.y * z)]);

                // Find currently used palette
                auto currentPaletteId = getVoxelPaletteId(glm::ivec3(x, y, z));

                // Find palette corresponding to palette ID
                int currentNodeStackIndex = 0;
                std::shared_ptr<MaterialPaletteNode> paletteNodeStack[Constants::VoxelWorld::paletteMapLayerCount + 1];
                paletteNodeStack[0] = palettes;

                for (int i = 0; i < Constants::VoxelWorld::paletteMapLayerCount; ++i)
                {
                    auto bitsShifted = 8 - (i << 2);
                    auto partialPaletteId = (currentPaletteId & (0b1111) << bitsShifted) >> bitsShifted;

                    auto& currentPaletteNode = paletteNodeStack[currentNodeStackIndex]->children[partialPaletteId];

                    currentNodeStackIndex++;
                    paletteNodeStack[currentNodeStackIndex] = currentPaletteNode;
                }

                // Check if palette contains desired material
                auto voxelIndex = x + size.x * (y + size.y * z);
                auto materialIndex = materialMap[voxelIndex];
                if (paletteNodeStack[currentNodeStackIndex]->materialIndices.contains(materialIndex))
                {
                    // If current palette contains the desired material, then we can move on
                    continue;
                }

                // Check if the palette has remaining space
                if (!paletteNodeStack[currentNodeStackIndex]->isFull())
                {
                    // If the current palette has remaining space, add it to the palette
                    for (const auto& node : paletteNodeStack)
                    {
                        node->materialIndices.emplace(materialIndex);
                    }

                    materialManager.materialIndexByPaletteId[paletteNodeStack[currentNodeStackIndex]->id] = materialIndex;
                }
                else
                {
                    // Otherwise, we try to change palettes
                    // This needs knowledge of the 4x4x4 and 16x16x16 regions

                    // Likely incorrect:
                    // Go up a level and repeat
                    // Once we successfully add a material to the palette, we need to add the material id downwards and update the mapping

                    Log::log("TODO: Change palettes");
                }

                // TODO: Implement modifying/changing palettes

                setVoxelPaletteId(glm::ivec3(x, y, z), paletteNodeStack[currentNodeStackIndex]->id);
            }
        }
    }

    materialManager.updateGpuMaterialData();
}

void VoxelWorldData::clearOccupancyMap()
{
    std::fill(occupancyMap.begin(), occupancyMap.end(), 0);
}

void VoxelWorldData::clearMaterialMap()
{
    std::fill(materialMap.begin(), materialMap.end(), 0);
}

void VoxelWorldData::clearPaletteMap()
{
    std::fill(paletteMap.begin(), paletteMap.end(), 0);
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
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, paletteMapIndices.at(paletteMapIndices.size() - 1), paletteMap.data());
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
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, paletteMapIndices.at(paletteMapIndices.size() - 1), paletteMap.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    world.updateMipMaps();
}
