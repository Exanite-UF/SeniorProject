#include "MaterialPalette.h"

#include <algorithm>

#include <src/Constants.h>
#include <src/utilities/Assert.h>

const std::vector<uint8_t>& MaterialPalette::getIds()
{
    return ids;
}

void MaterialPalette::addId(uint8_t id)
{
    Assert::isTrue(ids.size() <= Constants::VoxelWorld::materialsPerRegion, "Failed to add id: Material palettes can only have " + std::to_string(Constants::VoxelWorld::materialsPerRegion) + " ids");

    ids.push_back(id);
}

bool MaterialPalette::hasId(uint8_t id)
{
    return std::find(ids.begin(), ids.end(), id) != ids.end();
}

void MaterialPalette::clear()
{
    ids.clear();
}
