#pragma once

#include <src/utilities/OpenGl.h>
#include <src/utilities/Singleton.h>

class VoxelChunkResources : public Singleton<VoxelChunkResources>
{
public:
    GLuint makeNoiseComputeProgram = 0;
    GLuint makeMipMapComputeProgram = 0;
    GLuint assignMaterialComputeProgram = 0;

    VoxelChunkResources();
};
