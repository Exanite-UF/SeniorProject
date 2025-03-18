#pragma once

#include <src/utilities/Singleton.h>

// Not really a manager currently. Just a class to hold the shaders used by VoxelWorld.
class VoxelWorldManager : public Singleton<VoxelWorldManager>
{
public:
    GLuint makeNoiseComputeProgram = 0;
    GLuint makeMipMapComputeProgram = 0;
    GLuint assignMaterialComputeProgram = 0;

    VoxelWorldManager();
};
