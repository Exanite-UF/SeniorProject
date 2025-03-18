#pragma once

#include <memory>

#include <src/utilities/Singleton.h>
#include <src/world/SceneComponent.h>
#include <src/world/VoxelChunkComponent.h>

class VoxelChunkManager : public Singleton<VoxelChunkManager>
{
private:
    bool isInitialized = false;
    std::shared_ptr<SceneComponent> scene;

public:
    void initialize(const std::shared_ptr<SceneComponent>& scene);

    void update();
};
