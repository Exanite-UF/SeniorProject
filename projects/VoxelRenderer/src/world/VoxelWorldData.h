#pragma once

#include <memory>

#include <src/world/Material.h>
#include <src/world/VoxelWorld.h>

class VoxelWorldData : public NonCopyable
{
private:
    glm::ivec3 size = glm::ivec3(0);

    // Same format as on the GPU
    std::vector<uint8_t> occupancyMap {};
    std::vector<uint32_t> occupancyMapIndices {};

    // Same format as on the GPU
    std::vector<uint8_t> paletteMap {};
    std::array<GLuint, Constants::VoxelWorld::paletteMapLayerCount + 1> paletteMapIndices {};

    // Flattened material ID format
    // Must be encoded before sending to the GPU
    std::vector<uint16_t> materialMap {};

public:
    [[nodiscard]] const glm::ivec3& getSize() const;
    void setSize(const glm::ivec3& size);

    [[nodiscard]] bool getVoxelOccupancy(const glm::ivec3& position) const;
    void setVoxelOccupancy(const glm::ivec3& position, bool isOccupied);

    [[nodiscard]] const std::shared_ptr<Material>& getVoxelMaterial(glm::ivec3 position) const;
    void setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material);
    void setVoxelMaterial(const glm::ivec3& position, uint16_t materialIndex);

    [[nodiscard]] uint8_t getVoxelPartialPaletteId(const glm::ivec3& position, int level) const;
    void setVoxelPartialPaletteId(const glm::ivec3& position, uint8_t partialPaletteId, int level); // TODO: This bypasses change tracking

    [[nodiscard]] uint16_t getVoxelPaletteId(const glm::ivec3& position) const;
    void setVoxelPaletteId(const glm::ivec3& position, uint16_t paletteId); // TODO: This bypasses change tracking
    void setVoxelPaletteId(const glm::ivec3& position, uint8_t palette0, uint8_t palette1, uint8_t palette2); // TODO: This bypasses change tracking

    void decodePaletteMap(); // TODO: This bypasses change tracking
    void encodePaletteMap();

    void clearOccupancyMap();
    void clearMaterialMap();
    void clearPaletteMap(); // TODO: This bypasses change tracking

    void copyFrom(VoxelWorld& world);
    void writeTo(VoxelWorld& world);
};
