#include "VoxelChunkManager.h"

#include <src/utilities/Assert.h>
#include <src/utilities/Log.h>

void VoxelChunkManager::initialize(const std::shared_ptr<SceneComponent>& scene)
{
    Assert::isTrue(!isInitialized, "VoxelChunkManager has already been initialized");

    isInitialized = true;

    this->scene = scene;

    Log::log("Initialized VoxelChunkManager");
}

void VoxelChunkManager::update()
{
}
