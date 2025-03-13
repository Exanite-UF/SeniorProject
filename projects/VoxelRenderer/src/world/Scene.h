#pragma once

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/world/Camera.h>
#include <src/world/VoxelWorld.h>

class Scene : public Component
{
public:
    // TODO: Implement transformation hierarchy
    std::shared_ptr<Camera> camera {};
    std::vector<std::shared_ptr<VoxelWorld>> worlds {};
};
