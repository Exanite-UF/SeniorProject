#pragma once

#include <memory>

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
    [[nodiscard]] const glm::ivec3& getSize() const;
    void setSize(glm::ivec3 size);

    void setVoxelOccupancy(glm::ivec3 position, bool isOccupied);
    void setVoxelMaterial(glm::ivec3 position, const std::shared_ptr<Material>& material);
    void setVoxelMaterial(glm::ivec3 position, uint16_t materialIndex);

    [[nodiscard]] uint8_t getVoxelPartialMaterialId(glm::ivec3 position, int level) const;
    void setVoxelPartialMaterialId(glm::ivec3 position, uint8_t materialId, int level); // TODO: This bypasses change tracking

    [[nodiscard]] uint16_t getVoxelMippedMaterialId(glm::ivec3 position) const;
    void setVoxelMippedMaterialId(glm::ivec3 position, uint8_t material0, uint8_t material1, uint8_t material2); // TODO: This bypasses change tracking

    void decodeMaterialMipMap(); // TODO: This bypasses change tracking
    void encodeMaterialMipMap();

    void copyFrom(VoxelWorld& world);
    void writeTo(VoxelWorld& world);

    void clearOccupancy();
    void clearMaterials();
    void clearMaterialMipMap(); // TODO: This bypasses change tracking
};
