#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/world/CameraComponent.h>
#include <src/world/VoxelChunkComponent.h>

class SceneComponent : public Component
{
public:
    std::shared_ptr<CameraComponent> camera {};
    std::vector<std::shared_ptr<VoxelChunkComponent>> worlds {};
};
