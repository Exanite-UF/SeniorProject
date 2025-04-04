#pragma once

#include <memory>

#include <src/world/Material.h>
#include <src/world/VoxelChunk.h>

class VoxelChunkData : public NonCopyable
{
private:
    // This allows for easier copies without allowing automatic copies
    struct Data
    {
    public:
        glm::ivec3 size = glm::ivec3(0);

        // Same format as on the GPU
        std::vector<uint8_t> occupancyMap {};
        std::vector<uint32_t> occupancyMapIndices {};

        // Same format as on the GPU
        std::vector<uint16_t> materialMap {};
    };

    Data data;

public:
    VoxelChunkData();
    explicit VoxelChunkData(const glm::ivec3& size);

    [[nodiscard]] const glm::ivec3& getSize() const;
    void setSize(const glm::ivec3& size);

    [[nodiscard]] bool getVoxelOccupancy(const glm::ivec3& position) const;
    void setVoxelOccupancy(const glm::ivec3& position, bool isOccupied);

    [[nodiscard]] const std::shared_ptr<Material>& getVoxelMaterial(const glm::ivec3& position) const;
    void setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material);

    [[nodiscard]] uint16_t getVoxelMaterialIndex(const glm::ivec3& position) const;
    void setVoxelMaterialIndex(const glm::ivec3& position, uint16_t materialIndex);

    void clearOccupancyMap();
    void clearMaterialMap();

    void copyFrom(VoxelChunk& chunk);
    void writeTo(VoxelChunk& chunk);

    void copyFrom(VoxelChunkData& data);
    void writeTo(VoxelChunkData& data);
};
