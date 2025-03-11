#pragma once

#include <array>
#include <memory>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/Singleton.h>
#include <src/world/Material.h>
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

    // This maps material string key to the actual material
    // Speeds up getting materials by string key
    std::unordered_map<std::string, std::shared_ptr<Material>> materialsByKey {};

    // ----- GPU data -----
    // This maps the palette ID to the index of the actual material
    // This uses uint32_t instead of uint16_t since the GPU can only address individual uint32s
    // This corresponds to the data stored by materialMapBuffer
    std::array<uint32_t, Constants::VoxelWorld::palette0Count> materialIndexByPaletteId {};

    // This is the materials array converted from the CPU Material class to the GPU MaterialData struct
    // This corresponds to the data stored by materialDataBuffer
    std::array<MaterialData, Constants::VoxelWorld::materialCount> materialData {};

    GraphicsBuffer<MaterialData> materialDataBuffer = GraphicsBuffer<MaterialData>(Constants::VoxelWorld::materialCount);

    MaterialManager();

public:
    const std::shared_ptr<Material>& getMaterialByIndex(uint16_t index);
    const std::shared_ptr<Material>& getMaterialByKey(const std::string& key);
    bool tryGetMaterialByKey(const std::string& key, std::shared_ptr<Material>& material);

    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();

    void updateGpuMaterialData();

private:
    std::shared_ptr<Material>& createMaterial(const std::string& key, const std::string& name);
};
