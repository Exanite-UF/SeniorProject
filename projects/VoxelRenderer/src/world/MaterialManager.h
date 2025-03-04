#pragma once

#include "MaterialPalette.h"

#include <array>
#include <memory>
#include <vector>

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
    // ----- CPU -----
    // These arrays store different data, but are named materials# because they represent different layers of the material mipmaps
    std::array<std::shared_ptr<Material>, Constants::VoxelWorld::materialCount> materials0 {};
    std::array<std::shared_ptr<MaterialPalette>, Constants::VoxelWorld::materialMippedId1Count> materials1 {};
    std::array<std::shared_ptr<MaterialPalette>, Constants::VoxelWorld::materialMippedId2Count> materials2 {};

    std::unordered_map<std::string, std::shared_ptr<Material>> materialsById {};

    // ----- GPU -----
    // This uses uint32_t instead of uint16_t since the GPU can only address individual uint32s
    std::array<uint32_t, Constants::VoxelWorld::materialMippedId0Count> materialMap {};

    // Stores the GPU encoded material data
    std::array<MaterialData, Constants::VoxelWorld::materialCount> materialData {};

    GraphicsBuffer<uint32_t> materialMapBuffer = GraphicsBuffer<uint32_t>(Constants::VoxelWorld::materialMippedId0Count);
    GraphicsBuffer<MaterialData> materialDataBuffer = GraphicsBuffer<MaterialData>(Constants::VoxelWorld::materialCount);

    MaterialManager();

public:
    uint32_t getMaterialIndexByMipMappedId(uint16_t mipMapId) const;
    uint32_t getMaterialIndexByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2) const;

    const std::shared_ptr<Material>& getMaterialByMipMappedId(uint16_t mipMapId);
    const std::shared_ptr<Material>& getMaterialByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2);
    const std::shared_ptr<Material>& getMaterialByIndex(uint16_t index);
    const std::shared_ptr<Material>& getMaterialById(std::string id);
    bool tryGetMaterialById(std::string id, std::shared_ptr<Material>& material);

    GraphicsBuffer<uint32_t>& getMaterialMapBuffer();
    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();

    void updateGpuMaterialData();
};
