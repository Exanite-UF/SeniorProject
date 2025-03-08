#pragma once

#include <memory>
#include <vector>

#include <src/utilities/NonCopyable.h>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelWorld.h> //temp

class Scene : public Component
{
public:

    // TODO: Implement transformation hierarchy
    std::vector<std::shared_ptr<VoxelWorld>> worlds {};

    //std::shared_ptr<Camera> camera = std::make_shared<Camera>();
};
