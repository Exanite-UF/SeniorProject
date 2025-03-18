#pragma once

#include <memory>
#include <src/Constants.h>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelChunk.h>

class VoxelChunkComponent : public Component
{
    friend class GameObject;

private:
    std::shared_ptr<VoxelChunk> chunk;

public:
    explicit VoxelChunkComponent(glm::ivec3 worldSize = Constants::VoxelChunkComponent::chunkSize);

    std::shared_ptr<VoxelChunk>& getChunk();
};
