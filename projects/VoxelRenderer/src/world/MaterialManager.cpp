#include "MaterialManager.h"

#include <array>
#include <cmath>
#include <span>
#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>

MaterialManager::MaterialManager()
{
    // Define custom materials
    {
        auto& material = createMaterial("dirt", "Dirt");
        material->albedo = ColorUtility::srgbToLinear("#70381c");
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("blue_light", "Blue Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::srgbToLinear("#09e4e8");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("red_light", "Red Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::srgbToLinear("#ff0000");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    // Generate placeholder materials
    for (size_t i = createdMaterialCount; i < materials.size(); i++)
    {
        auto& material = createMaterial("generated_" + std::to_string(i), "Generated Material (Index " + std::to_string(i) + ") ");
        if (i % 4 == 0)
        {
            material->emission = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            material->albedo = material->emission;
            material->emission *= 0.2;
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
    for (size_t i = 0; i < materialIndexByPaletteId.size(); i++)
    {
        auto material = getMaterialByIndex(i % Constants::VoxelWorld::materialCount);

        materialIndexByPaletteId[i] = material->getIndex();
        material->ids.push_back(i);
    }

    updateGpuMaterialData();
}

uint32_t MaterialManager::getMaterialIndexByPaletteId(uint16_t paletteId) const
{
    return materialIndexByPaletteId[paletteId];
}

uint32_t MaterialManager::getMaterialIndexByPaletteId(uint8_t palette0, uint8_t palette1, uint8_t palette2) const
{
    uint32_t id = ((palette0 & 0b1111) << 0) | ((palette1 & 0b1111) << 4) | ((palette2 & 0b1111) << 8);
    return materialIndexByPaletteId[id];
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByPaletteId(uint16_t paletteId)
{
    return getMaterialByIndex(getMaterialIndexByPaletteId(paletteId));
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByPaletteId(uint8_t palette0, uint8_t palette1, uint8_t palette2)
{
    return getMaterialByIndex(getMaterialIndexByPaletteId(palette0, palette1, palette2));
}

const std::shared_ptr<Material>& MaterialManager::getMaterialByIndex(uint16_t index)
{
    return materials[index];
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

GraphicsBuffer<uint32_t>& MaterialManager::getMaterialIndicesByPaletteIdBuffer()
{
    return materialIndicesByPaletteIdBuffer;
}

GraphicsBuffer<MaterialData>& MaterialManager::getMaterialDataBuffer()
{
    return materialDataBuffer;
}

void MaterialManager::updateGpuMaterialData()
{
    // Convert CPU material format to GPU material format
    // TODO: Optimize this to only convert changed materials
    for (size_t i = 0; i < materials.size(); i++)
    {
        auto& material = materials[i];
        auto materialDataEntry = MaterialData();
        materialDataEntry.emission = material->emission;
        materialDataEntry.albedo = material->albedo;
        materialDataEntry.metallicAlbedo = material->metallicAlbedo;
        materialDataEntry.roughness = material->roughness;
        materialDataEntry.metallic = material->metallic;

        materialData[i] = materialDataEntry;
    }

    // Write data to GPU
    materialIndicesByPaletteIdBuffer.readFrom(materialIndexByPaletteId);
    materialDataBuffer.readFrom(materialData);
}

std::shared_ptr<Material>& MaterialManager::createMaterial(const std::string& key, const std::string& name)
{
    Assert::isTrue(createdMaterialCount < materials.size(), "Failed to add material: Too many materials defined");

    auto index = createdMaterialCount;
    createdMaterialCount++;

    auto material = std::make_shared<Material>(index, key);
    material->name = name;

    materials[index] = material;
    materialsByKey.emplace(key, material);

    return materials[index];
}
