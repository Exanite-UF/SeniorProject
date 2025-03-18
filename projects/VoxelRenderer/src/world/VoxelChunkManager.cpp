#include "VoxelChunkManager.h"

#include <src/Content.h>
#include <src/graphics/ShaderManager.h>

VoxelChunkManager::VoxelChunkManager()
{
    auto& shaderManager = ShaderManager::getInstance();

    makeNoiseComputeProgram = shaderManager.getComputeProgram(Content::makeNoiseComputeShader);
    makeMipMapComputeProgram = shaderManager.getComputeProgram(Content::makeMipMapComputeShader);
    assignMaterialComputeProgram = shaderManager.getComputeProgram(Content::assignMaterialComputeShader);
}
