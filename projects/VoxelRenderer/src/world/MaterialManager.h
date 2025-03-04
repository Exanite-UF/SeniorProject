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
    // ----- CPU data -----
    // These arrays store different data, but are named materials# because they represent different layers of the material mipmaps
    std::array<std::shared_ptr<Material>, Constants::VoxelWorld::materialCount> materials0 {};
    std::array<std::shared_ptr<MaterialPalette>, Constants::VoxelWorld::materialMippedId1Count> materials1 {};
    std::array<std::shared_ptr<MaterialPalette>, Constants::VoxelWorld::materialMippedId2Count> materials2 {};

    // TODO: Implement these and other count tracking variables
    // These track the number of materials/palettes used
    // Each correspond to one of the arrays above
    uint32_t material0Count = 0;
    uint32_t material1Count = 0;
    uint32_t material2Count = 0;

    // This maps mipped material ID to the index of the actual material
    // This uses uint32_t instead of uint16_t since the GPU can only address individual uint32s
    // This corresponds to the data stored by materialMapBuffer
    std::array<uint32_t, Constants::VoxelWorld::materialMippedId0Count> materialIdToIndexMap {};

    // This maps material string key to the actual material
    // Speeds up getting materials by string key
    std::unordered_map<std::string, std::shared_ptr<Material>> materialsByKey {};

    // ----- GPU data -----
    // This is the materials0 array converted from the CPU Material class to the GPU MaterialData struct
    // This corresponds to the data stored by materialDataBuffer
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
    const std::shared_ptr<Material>& getMaterialByKey(const std::string& key);
    bool tryGetMaterialByKey(const std::string& key, std::shared_ptr<Material>& material);

    GraphicsBuffer<uint32_t>& getMaterialMapBuffer();
    GraphicsBuffer<MaterialData>& getMaterialDataBuffer();

    void updateGpuMaterialData();
};
