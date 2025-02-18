#include "MaterialManager.h"

#include <array>
#include <span>

MaterialManager::MaterialManager()
{
    for (size_t i = 0; i < materials.size(); i++)
    {
        auto material = Material();
        // TODO: Set placeholder material properties

        materials[i] = material;
    }

    for (size_t i = 0; i < materialMap.size(); i++)
    {
        materialMap[i] = i % Constants::VoxelWorld::materialCount;
    }

    writeToGpu();
}

MaterialManager::~MaterialManager()
{
}

void MaterialManager::writeToGpu()
{
    // Convert CPU material format to GPU material format
    // TODO: Optimize this to only convert changed materials
    for (size_t i = 0; i < materials.size(); i++)
    {
        auto& material = materials[i];
        auto materialDataEntry = MaterialData();
        materialDataEntry.emission = material.emission;
        materialDataEntry.albedo = material.albedo;
        materialDataEntry.metallicAlbedo = material.metallicAlbedo;
        materialDataEntry.roughness = material.roughness;
        materialDataEntry.metallic = material.metallic;

        materialData[i] = materialDataEntry;
    }

    // Write data to GPU
    materialMapBuffer.readFrom(materialMap);
    materialDataBuffer.readFrom(materialData);
}

GraphicsBuffer<uint32_t>& MaterialManager::getMaterialMapBuffer()
{
    return materialMapBuffer;
}

GraphicsBuffer<MaterialData>& MaterialManager::getMaterialDataBuffer()
{
    return materialDataBuffer;
}
