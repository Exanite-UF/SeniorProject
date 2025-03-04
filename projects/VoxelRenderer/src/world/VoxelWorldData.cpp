#include "VoxelWorldData.h"

#include <src/utilities/Assert.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>
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

void VoxelWorldData::clearMaterials()
{
    std::fill(flattenedMaterialMap.begin(), flattenedMaterialMap.end(), 0);
}

void VoxelWorldData::clearMaterialMipMap()
{
    std::fill(materialMap.begin(), materialMap.end(), 0);
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

void VoxelWorldData::setVoxelMaterial(glm::ivec3 position, const std::shared_ptr<Material> material)
{
    setVoxelMaterial(position, material->getIndex());
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

    // Voxels correspond to 1x1x1 regions
    // This won't always be the position of the original voxel because of mipmapping
    auto voxelPosition = position;

    // Cells correspond to 2x2x2 regions
    auto cellCount = size >> 1;
    auto cellPosition = position >> 1;

    // Note: The position/count calculations above can be done using a bitshift operation, but that apparently is slower

    // Iterate each mipmap level
    // Note that each mipmap level increases region size by 4 times
    for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::materialMapLayerCount; ++mipMapI)
    {
        // Calculate uint index of cell
        auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z) + (materialMapIndices.at(mipMapI) / 4);

        // Calculate which set of 4-bits to set
        auto oddX = voxelPosition.x & 1;
        auto oddY = voxelPosition.y & 1;
        auto oddZ = voxelPosition.z & 1;
        auto bitsShifted = ((oddX << 0) | (oddY << 1) | (oddZ << 2)) << 2;

        // Get value that should be stored for this voxel
        auto voxelValue = materialIdParts[mipMapI];

        // Set data
        auto cellData = reinterpret_cast<uint32_t*>(materialMap.data());
        cellData[cellIndex] &= ~(0b1111 << bitsShifted);
        cellData[cellIndex] |= voxelValue << bitsShifted;

        // Update region size
        voxelPosition >>= 2;
        cellCount >>= 2;
        cellPosition >>= 2;
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
                // This stores the mipmapped material ID, not the material index
                uint16_t materialMipMappedId = 0;

                // Voxels correspond to 1x1x1 regions
                // This won't always be the position of the original voxel because of mipmapping
                auto voxelPosition = glm::ivec3(x, y, z);

                // Cells correspond to 2x2x2 regions
                auto cellCount = size / 2;
                auto cellPosition = voxelPosition / 2;

                // Iterate each mipmap level
                // Note that each mipmap level increases region size by 4 times
                for (int mipMapI = 0; mipMapI < Constants::VoxelWorld::materialMapLayerCount; ++mipMapI)
                {
                    // Calculate uint index of cell
                    auto cellIndex = cellPosition.x + cellCount.x * (cellPosition.y + cellCount.y * cellPosition.z);
                    cellIndex += materialMapIndices.at(mipMapI) / 4;

                    // Calculate which set of 4-bits to get
                    auto oddX = x & 1;
                    auto oddY = y & 1;
                    auto oddZ = z & 1;
                    auto bitsShifted = ((oddX << 0) | (oddY << 1) | (oddZ << 2)) << 2;

                    // Get value from cell and extract the 4 bit segment that we want
                    auto cellValue = reinterpret_cast<uint32_t*>(materialMap.data())[cellIndex];
                    auto voxelValue = cellValue & (0b1111 << bitsShifted);

                    // Store voxel value as part of result
                    materialMipMappedId |= (voxelValue >> bitsShifted) << (mipMapI * 4);

                    // Update region size
                    voxelPosition >>= 2;
                    cellCount >>= 2;
                    cellPosition >>= 2;
                }

                // setVoxelMaterial(glm::ivec3(x, y, z), materialMipMappedId); // For debugging. This lets you see the mipMappedId.
                setVoxelMaterial(glm::ivec3(x, y, z), materialManager.getMaterialIndexByMipMappedId(materialMipMappedId));
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

                setVoxelMipMappedMaterial(glm::ivec3(x, y, z), material0, material1, material2);
            }
        }
    }
}
