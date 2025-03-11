#include "MaterialPaletteNode.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <src/Constants.h>

MaterialPaletteNode::MaterialPaletteNode(int level, uint8_t childIndex, uint16_t parentId)
{
    this->level = level;
    this->childIndex = childIndex;
    id = (parentId << 4) | childIndex;

    if (level > 0)
    {
        for (uint8_t i = 0; i < Constants::VoxelWorld::palettesPerRegion; ++i)
        {
            children.emplace_back(std::make_shared<MaterialPaletteNode>(level - 1, i, id));
        }
    }

    maxMaterialIndexCount = std::pow(Constants::VoxelWorld::palettesPerRegion, level);
}

void MaterialPaletteNode::addMaterialIndex(uint16_t materialIndex)
{
    auto entry = materialIndices.insert({ materialIndex, 0 });

    // entry is a pair(inserted_entry, is_success)
    // Therefore, to increment the count, we have to increment the inserted_entry's second value
    entry.first->second++;
}

void MaterialPaletteNode::removeMaterialIndex(uint16_t materialIndex)
{
    auto entry = materialIndices.find(materialIndex);
    if (entry == materialIndices.end())
    {
        throw std::runtime_error("Failed to find material index to remove");
    }

    entry->second--;
    if (entry->second <= 0)
    {
        materialIndices.erase(entry);
    }
}

bool MaterialPaletteNode::hasMaterialIndex(uint16_t materialIndex)
{
    return materialIndices.contains(materialIndex);
}

size_t MaterialPaletteNode::getMaterialIndexCount()
{
    return materialIndices.size();
}

size_t MaterialPaletteNode::getMaxMaterialIndexCount()
{
    return maxMaterialIndexCount;
}

bool MaterialPaletteNode::isFull() const
{
    return materialIndices.size() >= maxMaterialIndexCount;
}

void MaterialPaletteNode::clear()
{
    materialIndices.clear();

    for (uint8_t i = 0; i < children.size(); ++i)
    {
        children[i]->clear();
    }
}
