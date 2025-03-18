#pragma once

#include <memory>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelWorld.h>

class VoxelChunkComponent : public Component
{
public:
    std::shared_ptr<VoxelWorld> world;
};
