#include "MaterialManager.h"

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

    {
        auto& material = createMaterial("yellow_light", "Yellow Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::srgbToLinear("#ffff00");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("green_light", "Green Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::srgbToLinear("#00ff00");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    // Generate placeholder materials
    for (size_t i = materials.size(); i < Constants::VoxelChunk::maxMaterialCount; i++)
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

    updateGpuMaterialData();
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

GraphicsBuffer<MaterialDefinition>& MaterialManager::getMaterialDefinitionsBuffer()
{
    return materialDefinitionsBuffer;
}

void MaterialManager::updateGpuMaterialData()
{
    // Convert CPU material format to GPU material format
    // TODO: Optimize this to only convert changed materials
    materialData.resize(materials.size());
    for (size_t i = 0; i < materials.size(); i++)
    {
        auto& material = materials[i];
        auto materialDataEntry = MaterialDefinition();
        materialDataEntry.emission = material->emission;
        materialDataEntry.albedo = material->albedo;
        materialDataEntry.metallicAlbedo = material->metallicAlbedo;
        materialDataEntry.roughness = material->roughness;
        materialDataEntry.metallic = material->metallic;

        materialData[i] = materialDataEntry;
    }

    // Write data to GPU
    materialDefinitionsBuffer.readFrom(materialData);
}

std::shared_ptr<Material>& MaterialManager::createMaterial(const std::string& key, const std::string& name)
{
    Assert::isTrue(materials.size() < Constants::VoxelChunk::maxMaterialCount, "Failed to add material: Too many materials defined");

    auto& material = materials.emplace_back(std::make_shared<Material>(materials.size(), key));
    material->name = name;

    materialsByKey.emplace(key, material);

    return material;
}
