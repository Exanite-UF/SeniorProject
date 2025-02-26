#pragma once

#include <array>
#include <vector>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/Singleton.h>
#include <src/world/Material.h>

class MaterialManager : public Singleton<MaterialManager>
{
    friend class Singleton;

private:
    // This uses uint32_t instead of uint16_t since the GPU can only address individual uint32s
    std::array<uint32_t, Constants::VoxelWorld::materialMapCount> materialMap;
    std::array<Material, Constants::VoxelWorld::materialCount> materials;

    // Stores the GPU encoded material data
    std::array<MaterialData, Constants::VoxelWorld::materialCount> materialData;

    GraphicsBuffer<uint32_t> materialMapBuffer = GraphicsBuffer<uint32_t>(Constants::VoxelWorld::materialMapCount);
    GraphicsBuffer<MaterialData> materialDataBuffer = GraphicsBuffer<MaterialData>(Constants::VoxelWorld::materialCount);

    MaterialManager();

public:
    uint32_t getMaterialIndexByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2) const;
    Material& getMaterialByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2);

    Material& getMaterialByIndex(uint16_t index);

    GraphicsBuffer<uint32_t>& getMaterialMapBuffer();
    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();

    void writeToGpu();
};
