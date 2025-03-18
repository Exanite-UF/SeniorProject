#pragma once

#include <src/rendering/CameraRenderCommand.h>
#include <src/rendering/VoxelChunkRenderCommand.h>
#include <vector>

struct SceneRenderCommand
{
public:
    CameraRenderCommand camera {};

    std::vector<VoxelChunkRenderCommand> chunks {};
};
