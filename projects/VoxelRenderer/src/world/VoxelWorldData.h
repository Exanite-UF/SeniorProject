#pragma once

#include <src/world/Material.h>
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
    std::array<GLuint, Constants::VoxelWorld::materialMapLayerCount + 1> materialMapIndices;

    // Flattened material ID format
    // Must be encoded before sending to the GPU
    std::vector<uint16_t> flattenedMaterialMap;

public:
    [[nodiscard]] const glm::ivec3& getSize();
    void setSize(glm::ivec3 size);

    void setVoxelOccupancy(glm::ivec3 position, bool isOccupied);
    void setVoxelMaterial(glm::ivec3 position, const Material& material);
    void setVoxelMaterial(glm::ivec3 position, const uint16_t material);
    void setVoxelMipMappedMaterial(glm::ivec3 position, uint8_t material0, uint8_t material1, uint8_t material2);

    void decodeMaterialMipMap();
    void encodeMaterialMipMap();

    void copyFrom(VoxelWorld& world);
    void writeTo(VoxelWorld& world);

    void clearOccupancy();
};
