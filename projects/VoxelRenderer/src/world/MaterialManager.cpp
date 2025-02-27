#include "MaterialManager.h"

#include <array>
#include <cmath>
#include <span>
#include <src/utilities/ColorUtility.h>

MaterialManager::MaterialManager()
{
    size_t customMaterialCount = 0;
    auto addMaterial = [&](std::string id, std::string name) -> Material&
    {
        auto index = customMaterialCount;
        customMaterialCount++;

        materials[index] = Material(index, name);
        return materials[index];
    };

    // Define custom materials
    auto& dirt = addMaterial("dirt", "Dirt");
    dirt.albedo = ColorUtility::srgbToLinear("#70381c");
    dirt.emission = glm::vec3(0);
    dirt.metallic = 0;
    dirt.metallicAlbedo = glm::vec3(0);
    dirt.roughness = 1;

    auto& blueLight = addMaterial("blue_light", "Blue Light");
    blueLight.albedo = glm::vec3(1);
    blueLight.emission = ColorUtility::srgbToLinear("#09e4e8");
    blueLight.metallic = 0;
    blueLight.metallicAlbedo = glm::vec3(0);
    blueLight.roughness = 1;

    // Generate placeholder materials
    for (size_t i = customMaterialCount; i < materials.size(); i++)
    {
        auto material = Material(i, "Material " + std::to_string(i));
        if (i % 4 == 0)
        {
            material.emission = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            material.albedo = material.emission;
            material.emission *= 0.5;
            // material.emission = glm::vec3(1, 1, 1);
            // material.emission *= glm::vec3(0.1, 0.1, 0.1);

            material.metallic = 0.0;
            material.metallicAlbedo = glm::vec3(0, 0, 0);
        }
        else
        {
            material.emission = glm::vec3(0, 0, 0);
            material.albedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            material.metallic = (rand() % 1000) / 1000.0;
            material.metallicAlbedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
        }

        material.roughness = (rand() % 1000) / 1000.0;

        materials[i] = material;
    }

    for (size_t i = 0; i < materialMap.size(); i++)
    {
        materialMap[i] = i % Constants::VoxelWorld::materialCount;
    }

    writeToGpu();
}

uint32_t MaterialManager::getMaterialIndexByMipMappedId(uint16_t mipMapId) const
{
    return materialMap[mipMapId];
}

uint32_t MaterialManager::getMaterialIndexByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2) const
{
    uint32_t id = ((material0 & 0b1111) << 0) | ((material1 & 0b1111) << 4) | ((material2 & 0b1111) << 8);
    return materialMap[id];
}

Material& MaterialManager::getMaterialByMipMappedId(uint16_t mipMapId)
{
    return getMaterialByIndex(getMaterialIndexByMipMappedId(mipMapId));
}

Material& MaterialManager::getMaterialByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2)
{
    return getMaterialByIndex(getMaterialIndexByMipMappedId(material0, material1, material2));
}

Material& MaterialManager::getMaterialByIndex(uint16_t index)
{
    return materials[index];
}

GraphicsBuffer<uint32_t>& MaterialManager::getMaterialMapBuffer()
{
    return materialMapBuffer;
}

GraphicsBuffer<MaterialData>& MaterialManager::getMaterialDataBuffer()
{
    return materialDataBuffer;
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
