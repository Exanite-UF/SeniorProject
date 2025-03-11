#include "VoxelWorldData.h"

#include <set>
#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>
#include <src/world/MaterialPaletteNodeStack.h>
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
    auto bitsShifted = (isOddPos.z << 2) | (isOddPos.y << 1) | (isOddPos.x << 0);
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

const std::shared_ptr<Material>& VoxelWorldData::getVoxelMaterial(glm::ivec3 position) const
{
    auto& materialManager = MaterialManager::getInstance();
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);

    return materialManager.getMaterialByIndex(materialMap[voxelIndex]);
}

void VoxelWorldData::setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material)
{
    setVoxelMaterialIndex(position, material->getIndex());
}

uint16_t VoxelWorldData::getVoxelMaterialIndex(glm::ivec3 position) const
{
    // Each material ID is 16 bits, but we only use the lower 12 bits
    auto voxelIndex = position.x + size.x * (position.y + size.y * position.z);
    return materialMap[voxelIndex];
}

void VoxelWorldData::setVoxelMaterialIndex(const glm::ivec3& position, const uint16_t materialIndex)
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
    auto bitsShifted = ((oddZ << 2) | (oddY << 1) | (oddX << 0)) << 2;

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
    auto bitsShifted = ((oddZ << 2) | (oddY << 1) | (oddX << 0)) << 2;

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
    for (int z = 0; z < size.z; ++z)
    {
        for (int y = 0; y < size.y; ++y)
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

                setVoxelMaterialIndex(glm::ivec3(x, y, z), materialIndex);
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

    // TODO: For debugging
    {
        // Reset palette map
        clearPaletteMap();

        // Reset material palettes
        palettes->clear();
        palettes->addMaterialIndex(0);
        palettes->children[0]->addMaterialIndex(0);
        palettes->children[0]->children[0]->addMaterialIndex(0);
        palettes->children[0]->children[0]->children[0]->addMaterialIndex(0);

        materialManager.materialIndexByPaletteId.fill(0);
    }

    // To figure out whether we can use a palette2, we need to see if the region2's desired materials can fit in the palette2
    // A palette2 is a set of sets of material indices. We'll call this a set^2.
    // A region2 is made up of region1s, each having a set of materials. This is effectively another set^2 of material indices.
    // The exact order of the sets don't matter, we just need to know if the desired set^2 of materials can be merged with the existing set^2 of materials
    //
    // However, we shouldn't modify the real data until we confirm that the two set^2s can be merged successfully.
    // Otherwise, we waste a lot of palettes (we solve them, then just drop them) or spend a lot of time tracking and undoing our operations.
    //
    // Whichever way we decide to verify that the two set^2s can be merged must also be used when we actually merge them
    // This means having two very similar blocks of code
    //
    // The algorithm is as follows:
    // Build a set^2 P for the current palette2, with each palette1's materials being a set within the set
    // Build a set^2 R for the region2's desired materials, with each region1's desired materials being a set within the set
    // For each set in R, merge with a set in P by doing the following:
    // 1. Loop through each set in P and calculate the number of materials that need to be added
    // 2. If the set from P has enough space, add the materials from the set from R to P
    // 3. If at any time we loop through all sets in P and fail to merge the set from R into a set from P, then we must make a new palette2
    //
    // (Edit: This is false; some sets can be merged) Warning: If R contains more than 16 sets, then merging will always fail
    // Not sure if it is better to attempt a best-attempt merge or to immediately start a new palette
    //
    // In any case, some materials must be discarded
    // To figure out which materials to be discarded, we need to count the number of times a material is used
    // This requires a data structure similar to a multiset. This is already implemented in MaterialPaletteNode using a std::unordered_map (not a std::multiset because that's slower).
    // We can then figure out which materials are used the "least" and remove those materials from R until R contains 16 or less sets.
    //
    // What defines the material that is used the least? And should it be tracked per region1 or per region2?

    // Region-based solver
    MaterialPaletteNodeStack stack(palettes);
    std::unordered_set<uint16_t> usedMaterialsSet3 {};
    auto palette2RegionCount = size >> 4;
    for (int palette2ZI = 0; palette2ZI < palette2RegionCount.z; ++palette2ZI)
    {
        for (int palette2YI = 0; palette2YI < palette2RegionCount.y; ++palette2YI)
        {
            for (int palette2XI = 0; palette2XI < palette2RegionCount.x; ++palette2XI)
            {
                // The code inside this block represents a 16x16x16 region
                std::unordered_set<uint16_t> usedMaterialsSet2 {};
                glm::ivec3 voxelPosition2 = glm::ivec3(palette2XI * 16, palette2YI * 16, palette2ZI * 16);
                stack.push(stack.getCurrent()->children[getVoxelPartialPaletteId(voxelPosition2, 2)]);

                for (int palette1ZI = 0; palette1ZI < 4; ++palette1ZI)
                {
                    for (int palette1YI = 0; palette1YI < 4; ++palette1YI)
                    {
                        for (int palette1XI = 0; palette1XI < 4; ++palette1XI)
                        {
                            // The code inside this block represents a 4x4x4 region
                            std::unordered_set<uint16_t> usedMaterialsSet1 {};
                            glm::ivec3 voxelPosition1 = voxelPosition2 + glm::ivec3(palette1XI * 4, palette1YI * 4, palette1ZI * 4);
                            stack.push(stack.getCurrent()->children[getVoxelPartialPaletteId(voxelPosition1, 1)]);

                            // Get materials used
                            for (int palette0ZI = 0; palette0ZI < 4; ++palette0ZI)
                            {
                                for (int palette0YI = 0; palette0YI < 4; ++palette0YI)
                                {
                                    for (int palette0XI = 0; palette0XI < 4; ++palette0XI)
                                    {
                                        glm::ivec3 voxelPosition0 = voxelPosition1 + glm::ivec3(palette0XI, palette0YI, palette0ZI);
                                        auto isOccupied = getVoxelOccupancy(voxelPosition0);
                                        if (!isOccupied)
                                        {
                                            // Ignore unoccupied voxels
                                            continue;
                                        }

                                        // Count materials used
                                        auto& material = getVoxelMaterial(voxelPosition0);
                                        usedMaterialsSet3.emplace(material->getIndex());
                                        usedMaterialsSet2.emplace(material->getIndex());
                                        usedMaterialsSet1.emplace(material->getIndex());
                                    }
                                }
                            }

                            Assert::isTrue(usedMaterialsSet1.size() <= 16, "Too many materials in a single palette1");

                            // We now have a list of materials used in a 4x4x4 region
                            // We will need to figure out if the palette has enough space
                            // This is done by seeing how many of these materials are in the currently used palette
                            std::vector usedMaterialsList1(usedMaterialsSet1.begin(), usedMaterialsSet1.end());

                            // Count number of materials that need to be added to the current palette
                            int materialsToAddCount = 0;
                            for (int i = 0; i < usedMaterialsList1.size(); ++i)
                            {
                                auto materialIndex = usedMaterialsList1[i];
                                if (!stack.getCurrent()->hasMaterialIndex(materialIndex))
                                {
                                    materialsToAddCount++;
                                }
                            }

                            // Check if the current palette can fit that many materials
                            if (stack.getCurrent()->getMaterialIndexCount() + materialsToAddCount > stack.getCurrent()->getMaxMaterialIndexCount())
                            {
                                // This means the current palette does not have enough space
                                // TODO
                                Assert::isTrue(false, "Not supported yet");
                            }

                            // Begin setting material palettes
                            // We can do this naively because we are guaranteed to have enough space due to the previous code
                            for (int palette0ZI = 0; palette0ZI < 4; ++palette0ZI)
                            {
                                for (int palette0YI = 0; palette0YI < 4; ++palette0YI)
                                {
                                    for (int palette0XI = 0; palette0XI < 4; ++palette0XI)
                                    {
                                        glm::ivec3 voxelPosition0 = voxelPosition1 + glm::ivec3(palette0XI, palette0YI, palette0ZI);
                                        auto isOccupied = getVoxelOccupancy(voxelPosition0);
                                        if (!isOccupied)
                                        {
                                            // Ignore unoccupied voxels
                                            continue;
                                        }

                                        Assert::isTrue(stack.getCount() == 3, "Expected 3 nodes on stack");
                                        stack.push(stack.getCurrent()->children[getVoxelPartialPaletteId(voxelPosition1, 0)]);
                                        {
                                            // Get desired material
                                            auto materialIndex = getVoxelMaterialIndex(voxelPosition0);

                                            // Check if the currently used palette contains desired material
                                            if (stack.getCurrent()->hasMaterialIndex(materialIndex))
                                            {
                                                // If current palette contains the desired material, then we can move on
                                                Assert::isTrue(stack.getCount() == 4, "Expected 4 nodes on stack");
                                                stack.pop();
                                                continue;
                                            }

                                            // Check if the palette has remaining space
                                            if (stack.getCurrent()->isFull())
                                            {
                                                // The current palette won't work since it is both full and does not contain the desired material
                                                // Therefore, we need to look for another palette0 in the parent palette1
                                                // Note: This is guaranteed to succeed

                                                stack.pop();

                                                // We first check if the material is already in the palette1
                                                auto& currentPalette1 = stack.getCurrent();
                                                if (currentPalette1->hasMaterialIndex(materialIndex))
                                                {
                                                    // We search for the palette0 that already has the material
                                                    for (auto child : currentPalette1->children)
                                                    {
                                                        if (child->hasMaterialIndex(materialIndex))
                                                        {
                                                            stack.push(child);

                                                            break;
                                                        }
                                                    }

                                                    Assert::isTrue(stack.getCount() == 4, "Expected 4 nodes on stack");

                                                    // Update palette map to use this palette
                                                    setVoxelPaletteId(voxelPosition0, stack.getCurrent()->id);
                                                }
                                                else
                                                {
                                                    // We find an empty palette0 to use and add the desired material to it
                                                    for (auto child : currentPalette1->children)
                                                    {
                                                        if (!child->isFull())
                                                        {
                                                            stack.push(child);

                                                            break;
                                                        }
                                                    }

                                                    Assert::isTrue(stack.getCount() == 4, "Expected 4 nodes on stack");

                                                    for (const auto& node : stack.getNodes())
                                                    {
                                                        node->addMaterialIndex(materialIndex);
                                                    }

                                                    // Register new palette-material mapping
                                                    materialManager.materialIndexByPaletteId[stack.getCurrent()->id] = materialIndex;

                                                    // Update palette map to use this palette
                                                    setVoxelPaletteId(voxelPosition0, stack.getCurrent()->id);
                                                }
                                            }
                                        }

                                        Assert::isTrue(stack.getCount() == 4, "Expected 4 nodes on stack");
                                        stack.pop();
                                    }
                                }
                            }

                            Assert::isTrue(stack.getCount() == 3, "Expected 3 nodes on stack");
                            stack.pop();
                        }
                    }
                }

                Assert::isTrue(usedMaterialsSet2.size() <= 256, "Too many materials in a single palette2");

                Assert::isTrue(stack.getCount() == 2, "Expected 2 nodes on stack");
                stack.pop();
            }
        }
    }

    // Incremental solver
    for (int z = 0; z < size.z; ++z)
    {
        for (int y = 0; y < size.y; ++y)
        {
            for (int x = 0; x < size.x; ++x)
            {
                auto isOccupied = getVoxelOccupancy(glm::ivec3(x, y, z));
                if (!isOccupied)
                {
                    // Ignore unoccupied voxels
                    continue;
                }

                // Find currently used palette
                auto currentPaletteId = getVoxelPaletteId(glm::ivec3(x, y, z));

                // Find palette corresponding to palette ID
                MaterialPaletteNodeStack stack(palettes);
                for (int i = 0; i < Constants::VoxelWorld::paletteMapLayerCount; ++i)
                {
                    auto bitsShifted = 8 - (i << 2);
                    auto partialPaletteId = (currentPaletteId & (0b1111) << bitsShifted) >> bitsShifted;

                    stack.push(stack.getCurrent()->children[partialPaletteId]);
                }

                // TODO: This should be part of the region solving loop
                // TODO: Start of region solving loop
                // Check if palette contains desired material
                auto materialIndex = getVoxelMaterialIndex(glm::ivec3(x, y, z));
                if (stack.getCurrent()->hasMaterialIndex(materialIndex))
                {
                    // If current palette contains the desired material, then we can move on
                    continue;
                }

                // Check if the palette has remaining space
                if (!stack.getCurrent()->isFull())
                {
                    // If the current palette has remaining space, add it to the palette
                    for (const auto& node : stack.getNodes())
                    {
                        node->addMaterialIndex(materialIndex);
                    }

                    // TODO: This code expects a palette0
                    materialManager.materialIndexByPaletteId[stack.getCurrent()->id] = materialIndex;
                }
                else
                {
                    // TODO: Use a loop
                    // currentNodeStackIndex--;

                    // Otherwise, we try to change palettes
                    // This needs knowledge of the 4x4x4 and 16x16x16 regions

                    // If a palette1 or palette2 is CHANGED, then we need to re-solve the region
                    // Adding a new child palette does not require re-solving

                    // Conclusion: It's best to implement region solving first
                    // The currently implemented approach is ideal for incremental changes, but not great for solving the entire chunk

                    // Likely incorrect:
                    // Go up a level and repeat
                    // Once we successfully add a material to the palette, we need to add the material id downwards and update the mapping

                    auto breakpointChangePalettes = 1; // TODO: For debugging
                }

                // TODO: Implement modifying/changing palettes

                // TODO: End of region solving loop

                setVoxelPaletteId(glm::ivec3(x, y, z), materialIndex);
            }
        }
    }

    // TODO: For debugging
    for (int i = 0; i < 16; ++i)
    {
        materialManager.materialIndexByPaletteId[i] = i % 16;
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
