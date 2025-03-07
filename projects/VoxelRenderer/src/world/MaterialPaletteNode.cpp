#include "MaterialPaletteNode.h"

#include <algorithm>

#include <src/Constants.h>

MaterialPaletteNode::MaterialPaletteNode(int level, uint8_t childIndex, uint16_t parentId)
{
    this->level = level;
    id = (parentId << 4) | childIndex;

    if (level > 0)
    {
        for (uint8_t i = 0; i < Constants::VoxelWorld::palettesPerRegion; ++i)
        {
            children.emplace_back(std::make_shared<MaterialPaletteNode>(level - 1, i, id));
        }
    }
}

void MaterialPaletteNode::clear()
{
    materialIndices.clear();

    for (uint8_t i = 0; i < children.size(); ++i)
    {
        children[i]->clear();
    }
}
