#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/world/CameraComponent.h>
#include <src/world/VoxelChunkComponent.h>

class SceneComponent : public Component
{
public:
    // The currently active camera
    std::shared_ptr<CameraComponent> camera {};
    std::vector<std::shared_ptr<VoxelChunkComponent>> chunks {};

    bool tryGetClosestChunk(std::shared_ptr<VoxelChunkComponent>& result);
};
