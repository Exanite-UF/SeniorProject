#include "MaterialManager.h"

#include <span>
#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>

MaterialManager::MaterialManager()
{
    // Define custom materials
    {
        auto& material = createMaterial("dirt", "Dirt");
        material->albedo = ColorUtility::htmlToLinear("#70381c"); // glm::vec3(1, 1, 1); //
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("stone", "stone");
        material->albedo = ColorUtility::htmlToLinear("#5F5F5F");
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("grass", "grass");
        material->albedo = ColorUtility::htmlToLinear("#636434");
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("oak_log", "oak_log");
        material->albedo = ColorUtility::htmlToLinear("#473621");
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("oak_leaf", "oak_leaf");
        material->albedo = ColorUtility::htmlToLinear("#434F1E");
        material->emission = glm::vec3(0);
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("blue_light", "Blue Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::htmlToLinear("#09e4e8");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("red_light", "Red Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::htmlToLinear("#ff0000");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("yellow_light", "Yellow Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::htmlToLinear("#ffff00");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    {
        auto& material = createMaterial("green_light", "Green Light");
        material->albedo = glm::vec3(1);
        material->emission = ColorUtility::htmlToLinear("#00ff00");
        material->metallic = 0;
        material->metallicAlbedo = glm::vec3(0);
        material->roughness = 1;
    }

    // Generate placeholder materials
    for (size_t i = materials.size(); i < Constants::VoxelChunk::maxMaterialCount; i++)
    {
        auto& material = createMaterial("generated_" + std::to_string(i), "Generated Material (Index " + std::to_string(i) + ") ");
        if ((rand() % 10000) / 10000.0 <= 0.1)
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
            material->metallic = 0 * (rand() % 1000) / 1000.0;
            material->metallicAlbedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
        }

        material->roughness = std::pow((rand() % 1000) / 1000.0, 2);
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

bool MaterialManager::tryGetMaterialByKey(const std::string& key, std::shared_ptr<Material>& outMaterial)
{
    auto entry = materialsByKey.find(key);
    if (entry == materialsByKey.end())
    {
        return false;
    }

    outMaterial = entry->second;
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
    materialDefinitionsBuffer.copyFrom(materialData);
}

std::shared_ptr<Material>& MaterialManager::createMaterial(const std::string& key, const std::string& name)
{
    Assert::isTrue(materials.size() < Constants::VoxelChunk::maxMaterialCount, "Failed to add material: Too many materials defined");

    auto& material = materials.emplace_back(std::make_shared<Material>(materials.size(), key));
    material->name = name;

    materialsByKey.emplace(key, material);

    return material;
}
