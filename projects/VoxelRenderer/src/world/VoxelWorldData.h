#pragma once

#include <src/world/VoxelWorld.h>

class VoxelWorldData : public NonCopyable
{
private:
    glm::ivec3 size;

    // Same format as on the GPU
    std::vector<uint8_t> occupancyMap;
    std::vector<uint32_t> occupancyMapIndices;

    // Same format as on the GPU
    std::vector<uint8_t> materialMap;
    std::array<GLuint, Constants::VoxelWorld::materialMapLayerCount + 1> materialIdMapIndices;

    // Flattened material ID format
    // Must be encoded before sending to the GPU
    std::vector<uint16_t> flattenedMaterialMap;

public:
    [[nodiscard]] const glm::ivec3& getSize();
    void setSize(glm::ivec3 size);

    void setVoxelOccupancy(glm::ivec3 position, bool isOccupied);

    void copyFrom(VoxelWorld& world);
    void writeTo(VoxelWorld& world);

    void clearOccupancy();
};
