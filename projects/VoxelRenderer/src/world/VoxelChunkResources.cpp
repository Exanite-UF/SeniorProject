#include "VoxelChunkResources.h"

#include <src/Content.h>
#include <src/graphics/ShaderManager.h>

VoxelChunkResources::VoxelChunkResources()
{
    auto& shaderManager = ShaderManager::getInstance();

    makeNoiseComputeProgram = shaderManager.getComputeProgram(Content::makeNoiseComputeShader)->programId;
    makeMipMapComputeProgram = shaderManager.getComputeProgram(Content::makeMipMapComputeShader)->programId;
    assignMaterialComputeProgram = shaderManager.getComputeProgram(Content::assignMaterialComputeShader)->programId;
}
