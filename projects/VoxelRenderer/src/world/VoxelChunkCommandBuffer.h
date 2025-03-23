#pragma once

#include <src/world/VoxelChunkComponent.h>

// A command buffer for commands related to modifying a VoxelChunkComponent
class VoxelChunkCommandBuffer
{
    // TODO: Implement this API

public:
    void setSize(const glm::ivec3& size);

    void setVoxelOccupancy(const glm::ivec3& position, bool isOccupied);
    void setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material);
    void setVoxelMaterialIndex(const glm::ivec3& position, uint16_t materialIndex);

    void clearOccupancyMap();
    void clearMaterialMap();

    void copyFrom(const std::shared_ptr<VoxelChunkData>& data);

public:
    void apply(const std::shared_ptr<VoxelChunkComponent>& chunk);
    void clear();
};
