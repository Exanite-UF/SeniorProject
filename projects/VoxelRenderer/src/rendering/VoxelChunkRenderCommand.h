#pragma once

#include <memory>
#include <src/world/VoxelChunk.h>

struct VoxelChunkRenderCommand
{
    std::shared_ptr<VoxelChunk> chunk;
};
