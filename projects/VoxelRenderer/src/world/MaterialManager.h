#pragma once

#include <array>
#include <vector>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/NonCopyable.h>
#include <src/world/Material.h>

class MaterialManager : public NonCopyable
{
private:
    std::array<uint32_t, Constants::VoxelWorld::materialMapCount> materialMap;
    std::array<Material, Constants::VoxelWorld::materialCount> materials;

    // Stores the GPU encoded material data
    std::array<MaterialData, Constants::VoxelWorld::materialCount> materialData;

    GraphicsBuffer<uint32_t> materialMapBuffer = GraphicsBuffer<uint32_t>(Constants::VoxelWorld::materialMapCount);
    GraphicsBuffer<MaterialData> materialDataBuffer = GraphicsBuffer<MaterialData>(Constants::VoxelWorld::materialCount);

    static MaterialManager* instance;

    MaterialManager();
    ~MaterialManager();

public:
    void writeToGpu();

    GraphicsBuffer<uint32_t>& getMaterialMapBuffer();
    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();

    static MaterialManager& getInstance();
};
