#pragma once

#include <memory>
#include <src/Constants.h>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelWorld.h>

class VoxelChunkComponent : public Component
{
public:
    std::shared_ptr<VoxelWorld> world;

    explicit VoxelChunkComponent(glm::ivec3 worldSize = Constants::VoxelChunkComponent::chunkSize);
};
