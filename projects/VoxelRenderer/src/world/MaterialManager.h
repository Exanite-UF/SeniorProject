#pragma once

#include <vector>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/NonCopyable.h>
#include <src/world/Material.h>

class MaterialManager : public NonCopyable
{
private:
    std::vector<uint32_t> materialMap;
    std::vector<Material> materials;

    // Stores the GPU encoded material data
    std::vector<MaterialData> materialData;

    GraphicsBuffer<uint32_t> materialMapBuffer = GraphicsBuffer<uint32_t>(Constants::VoxelWorld::materialMapCount);
    GraphicsBuffer<MaterialData> materialDataBuffer = GraphicsBuffer<MaterialData>(Constants::VoxelWorld::materialCount);

public:
    MaterialManager();
    ~MaterialManager();

    void writeToGpu();

    GraphicsBuffer<uint32_t>& getMaterialMapBuffer();
    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();
};
