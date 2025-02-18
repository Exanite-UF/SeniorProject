#pragma once

#include <vector>

#include <src/graphics/GraphicsBuffer.h>
#include <src/world/Material.h>

class MaterialManager
{
private:
    std::vector<uint32_t> materialMap;
    std::vector<Material> materials;

    // For the GPU
    std::vector<MaterialData> materialData;

    GraphicsBuffer<uint32_t> materialMapBuffer;
    GraphicsBuffer<MaterialData> materialDataBuffer;

public:
};
