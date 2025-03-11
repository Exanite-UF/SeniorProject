#pragma once

#include <cstdint>
#include <memory>
#include <src/utilities/NonCopyable.h>
#include <unordered_map>
#include <vector>

class MaterialPaletteNode : public NonCopyable
{
private:
    std::unordered_map<uint16_t, uint16_t> materialIndices {};
    int maxMaterialIndexCount;

public:
    int level;
    uint16_t id;
    uint8_t childIndex;

    std::vector<std::shared_ptr<MaterialPaletteNode>> children {};

    // Child index is at most 16 = 2^4
    MaterialPaletteNode(int level, uint8_t childIndex, uint16_t parentId);

    void addMaterialIndex(uint16_t materialIndex);
    void removeMaterialIndex(uint16_t materialIndex);
    bool hasMaterialIndex(uint16_t materialIndex);

    size_t getMaterialIndexCount();
    size_t getMaxMaterialIndexCount();

    bool isFull() const;

    void clear();
};
