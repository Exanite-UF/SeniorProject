#pragma once

#include <array>
#include <memory>
#include <unordered_set>
#include <vector>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/Singleton.h>
#include <src/world/Material.h>
#include <src/world/MaterialPaletteNode.h>
#include <src/world/VoxelWorldData.h>

class MaterialManager : public Singleton<MaterialManager>
{
    friend class Singleton;
    friend class VoxelWorldData;

private:
    // ----- CPU data -----
    // This tracks the number of created material definitions stored in the materials0 array
    uint16_t createdMaterialCount = 0;
    // This stores all material definitions
    std::array<std::shared_ptr<Material>, Constants::VoxelWorld::materialCount> materials {};

    std::shared_ptr<MaterialPaletteNode> palettes = std::make_shared<MaterialPaletteNode>(3, 0, 0);

    // This maps material string key to the actual material
    // Speeds up getting materials by string key
    std::unordered_map<std::string, std::shared_ptr<Material>> materialsByKey {};

    // ----- GPU data -----
    // This maps mipped material ID to the index of the actual material
    // This uses uint32_t instead of uint16_t since the GPU can only address individual uint32s
    // This corresponds to the data stored by materialMapBuffer
    std::array<uint32_t, Constants::VoxelWorld::palette0Count> materialIdToIndexMap {};

    // This is the materials0 array converted from the CPU Material class to the GPU MaterialData struct
    // This corresponds to the data stored by materialDataBuffer
    std::array<MaterialData, Constants::VoxelWorld::materialCount> materialData {};

    GraphicsBuffer<uint32_t> materialMapBuffer = GraphicsBuffer<uint32_t>(Constants::VoxelWorld::palette0Count);
    GraphicsBuffer<MaterialData> materialDataBuffer = GraphicsBuffer<MaterialData>(Constants::VoxelWorld::materialCount);

    MaterialManager();

public:
    uint32_t getMaterialIndexByPaletteId(uint16_t paletteId) const;
    uint32_t getMaterialIndexByPaletteId(uint8_t palette0, uint8_t palette1, uint8_t palette2) const;

    const std::shared_ptr<Material>& getMaterialByPaletteId(uint16_t paletteId);
    const std::shared_ptr<Material>& getMaterialByPaletteId(uint8_t palette0, uint8_t palette1, uint8_t palette2);
    const std::shared_ptr<Material>& getMaterialByIndex(uint16_t index);
    const std::shared_ptr<Material>& getMaterialByKey(const std::string& key);
    bool tryGetMaterialByKey(const std::string& key, std::shared_ptr<Material>& material);

    GraphicsBuffer<uint32_t>& getMaterialMapBuffer();
    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();

    void updateGpuMaterialData();

private:
    std::shared_ptr<Material>& createMaterial(const std::string& key, const std::string& name);
};
