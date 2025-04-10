#pragma once

#include <array>
#include <memory>
#include <unordered_set>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/Singleton.h>
#include <src/world/Material.h>
#include <src/world/VoxelChunkData.h>

class MaterialManager : public Singleton<MaterialManager>
{
    friend class Singleton;
    friend class VoxelChunkData;

private:
    // ----- CPU data -----
    // This stores all material definitions
    std::vector<std::shared_ptr<Material>> materials {};

    // This maps material string key to the actual material
    // Speeds up getting materials by string key
    std::unordered_map<std::string, std::shared_ptr<Material>> materialsByKey {};

    // This is the materials array converted from the CPU Material class to the GPU MaterialData struct
    // This corresponds to the data stored by materialDataBuffer
    std::vector<MaterialDefinition> materialData {};

    // ----- GPU data -----
    GraphicsBuffer<MaterialDefinition> materialDefinitionsBuffer = GraphicsBuffer<MaterialDefinition>(Constants::VoxelChunk::maxMaterialCount);

    MaterialManager();

public:
    const std::shared_ptr<Material>& getMaterialByIndex(uint16_t index);
    const std::shared_ptr<Material>& getMaterialByKey(const std::string& key);
    bool tryGetMaterialByKey(const std::string& key, std::shared_ptr<Material>& outMaterial);

    GraphicsBuffer<MaterialDefinition>& getMaterialDefinitionsBuffer();

    void updateGpuMaterialData();

private:
    std::shared_ptr<Material>& createMaterial(const std::string& key, const std::string& name);
};
