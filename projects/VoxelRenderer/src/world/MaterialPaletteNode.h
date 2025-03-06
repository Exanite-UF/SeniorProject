#pragma once

#include <cstdint>
#include <memory>
#include <src/utilities/NonCopyable.h>
#include <unordered_set>
#include <vector>

class MaterialPaletteNode : public NonCopyable
{
public:
    int level;
    uint16_t id;

    std::vector<std::shared_ptr<MaterialPaletteNode>> children {};
    std::unordered_set<uint16_t> materialIndices {};

    // Child index is at most 16 = 2^4
    MaterialPaletteNode(int level, uint8_t childIndex, uint16_t parentId);

    void clear();
};
