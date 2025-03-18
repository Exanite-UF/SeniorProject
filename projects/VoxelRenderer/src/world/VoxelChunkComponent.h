#pragma once

#include <memory>
#include <src/Constants.h>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelWorld.h>

class VoxelChunkComponent : public Component
{
    friend class GameObject;

private:
    std::shared_ptr<VoxelWorld> chunk;

public:
    explicit VoxelChunkComponent(glm::ivec3 worldSize = Constants::VoxelChunkComponent::chunkSize);

    std::shared_ptr<VoxelWorld>& getChunk();
};
