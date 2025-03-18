#pragma once

#include <src/utilities/OpenGl.h>
#include <src/utilities/Singleton.h>

// Not really a manager currently. Just a class to hold the shaders used by VoxelChunk.
class VoxelChunkManager : public Singleton<VoxelChunkManager>
{
public:
    GLuint makeNoiseComputeProgram = 0;
    GLuint makeMipMapComputeProgram = 0;
    GLuint assignMaterialComputeProgram = 0;

    VoxelChunkManager();
};
