#pragma once

#include <memory>
#include <vector>

#include <src/utilities/NonCopyable.h>
#include <src/world/Camera.h>
#include <src/world/VoxelWorld.h>

class Scene : public NonCopyable
{
public:
    // TODO: Implement transformation hierarchy

    std::vector<std::shared_ptr<VoxelWorld>> worlds {};
    std::shared_ptr<Camera> camera = std::make_shared<Camera>();
};
