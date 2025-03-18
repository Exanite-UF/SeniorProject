#pragma once

#include <memory>
#include <src/world/VoxelWorld.h>

struct VoxelChunkRenderCommand
{
    std::shared_ptr<VoxelWorld> voxelWorld;
};
