#include "MaterialManager.h"

#include <array>
#include <cmath>
#include <span>
#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>

MaterialManager::MaterialManager()
{
    // Create palettes
    for (int i = 0; i < materials1.size(); ++i)
    {
        materials1[i] = std::make_shared<MaterialPalette>();
    }

    for (int i = 0; i < materials2.size(); ++i)
    {
        materials2[i] = std::make_shared<MaterialPalette>();
    }

    // Add default palettes
    materials1[0]->addId(0);
    materials2[0]->addId(0);

    // Create addMaterial lambda
    size_t customMaterialCount = 0;
    auto addMaterial = [&](const std::string& key, const std::string& name) -> std::shared_ptr<Material>&
    {
        Assert::isTrue(customMaterialCount < materials0.size(), "Failed to add material: Too many materials defined");

        auto index = customMaterialCount;
        customMaterialCount++;

        auto material = std::make_shared<Material>(index, key);
        material->name = name;

        materials0[index] = material;
        materialsByKey.emplace(key, material);

        return materials0[index];
    };

    // Define custom materials
    {
        auto& material = addMaterial("dirt", "Dirt");
        material->albedo = ColorUtility::srgbToLinear("#70381c");
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = addMaterial("blue_light", "Blue Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::srgbToLinear("#09e4e8");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = addMaterial("red_light", "Red Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::srgbToLinear("#ff0000");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    // Generate placeholder materials
    for (size_t i = customMaterialCount; i < materials0.size(); i++)
    {
        auto& material = addMaterial("generated_" + std::to_string(i), "Generated Material (Index " + std::to_string(i) + ") ");
        if (i % 4 == 0)
        {
            material->emission = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            material->albedo = material->emission;
            material->emission *= 0.5;
            // material->emission = glm::vec3(1, 1, 1);
            // material->emission *= glm::vec3(0.1, 0.1, 0.1);

            material->metallic = 0.0;
            material->metallicAlbedo = glm::vec3(0, 0, 0);
        }
        else
        {
            material->emission = glm::vec3(0, 0, 0);
            material->albedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            material->metallic = (rand() % 1000) / 1000.0;
            material->metallicAlbedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
        }

        material->roughness = (rand() % 1000) / 1000.0;
    }

    // Generate placeholder material mappings
    for (size_t i = 0; i < materialIdToIndexMap.size(); i++)
    {
        auto material = getMaterialByIndex(i % Constants::VoxelWorld::materialCount);

        materialIdToIndexMap[i] = material->getIndex();
        material->ids.push_back(i);
    }

    updateGpuMaterialData();
}

uint32_t MaterialManager::getMaterialIndexByMipMappedId(uint16_t mipMapId) const
{
    return materialIdToIndexMap[mipMapId];
}

uint32_t MaterialManager::getMaterialIndexByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2) const
{
    uint32_t id = ((material0 & 0b1111) << 0) | ((material1 & 0b1111) << 4) | ((material2 & 0b1111) << 8);
    return materialIdToIndexMap[id];
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByMipMappedId(uint16_t mipMapId)
{
    return getMaterialByIndex(getMaterialIndexByMipMappedId(mipMapId));
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByMipMappedId(uint8_t material0, uint8_t material1, uint8_t material2)
{
    return getMaterialByIndex(getMaterialIndexByMipMappedId(material0, material1, material2));
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByIndex(uint16_t index)
{
    return materials0[index];
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByKey(const std::string& key)
{
    return materialsByKey.at(key);
}

bool MaterialManager::tryGetMaterialByKey(const std::string& key, std::shared_ptr<Material>& material)
{
    auto entry = materialsByKey.find(key);
    if (entry == materialsByKey.end())
    {
        return false;
    }

    material = entry->second;
    return true;
}

GraphicsBuffer<uint32_t>& MaterialManager::getMaterialMapBuffer()
{
    return materialMapBuffer;
}

GraphicsBuffer<MaterialData>& MaterialManager::getMaterialDataBuffer()
{
    return materialDataBuffer;
}

void MaterialManager::updateGpuMaterialData()
{
    // Convert CPU material format to GPU material format
    // TODO: Optimize this to only convert changed materials
    for (size_t i = 0; i < materials0.size(); i++)
    {
        auto& material = materials0[i];
        auto materialDataEntry = MaterialData();
        materialDataEntry.emission = material->emission;
        materialDataEntry.albedo = material->albedo;
        materialDataEntry.metallicAlbedo = material->metallicAlbedo;
        materialDataEntry.roughness = material->roughness;
        materialDataEntry.metallic = material->metallic;

        materialData[i] = materialDataEntry;
    }

    // Write data to GPU
    materialMapBuffer.readFrom(materialIdToIndexMap);
    materialDataBuffer.readFrom(materialData);
}
