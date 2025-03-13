#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/world/CameraComponent.h>
#include <src/world/VoxelWorld.h>

class SceneComponent : public Component
{
public:
    // TODO: Implement transformation hierarchy
    std::shared_ptr<CameraComponent> camera {};
    std::vector<std::shared_ptr<VoxelWorld>> worlds {};
};
