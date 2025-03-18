#pragma once

#include <memory>

#include <src/world/Material.h>
#include <src/world/VoxelChunk.h>

class VoxelChunkData : public NonCopyable
{
private:
    glm::ivec3 size = glm::ivec3(0);

    // Same format as on the GPU
    std::vector<uint8_t> occupancyMap {};
    std::vector<uint32_t> occupancyMapIndices {};

    // Same format as on the GPU
    std::vector<uint16_t> materialMap {};

public:
    [[nodiscard]] const glm::ivec3& getSize() const;
    void setSize(const glm::ivec3& size);

    [[nodiscard]] bool getVoxelOccupancy(const glm::ivec3& position) const;
    void setVoxelOccupancy(const glm::ivec3& position, bool isOccupied);

    [[nodiscard]] const std::shared_ptr<Material>& getVoxelMaterial(glm::ivec3 position) const;
    void setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material);

    [[nodiscard]] uint16_t getVoxelMaterialIndex(glm::ivec3 position) const;
    void setVoxelMaterialIndex(const glm::ivec3& position, uint16_t materialIndex);

    void clearOccupancyMap();
    void clearMaterialMap();

    void copyFrom(VoxelChunk& chunk);
    void writeTo(VoxelChunk& chunk);
};
