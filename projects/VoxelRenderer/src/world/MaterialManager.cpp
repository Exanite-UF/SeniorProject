#include "MaterialManager.h"

#include <array>
#include <span>

MaterialManager* MaterialManager::instance = nullptr;

MaterialManager& MaterialManager::getInstance()
{
    if (instance == nullptr)
    {
        instance = new MaterialManager();
    }

    return *instance;
}

MaterialManager::MaterialManager()
{
    for (size_t i = 0; i < materials.size(); i++)
    {
        auto material = Material();
        // TODO: Set placeholder material properties
        if(i % 4 == 0){
            material.emission = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            //material.emission = glm::vec3(1, 1, 1);
            //material.emission *= glm::vec3(0.1, 0.1, 0.1);
            material.albedo = glm::vec3(0, 0, 0);
            material.metallic = 0.0;
            material.metallicAlbedo = glm::vec3(0, 0, 0);
        }else{
            material.emission = glm::vec3(0, 0, 0);
            material.albedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
            material.metallic = (rand() % 1000) / 1000.0;
            material.metallicAlbedo = glm::vec3((rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0, (rand() % 1000) / 1000.0);
        }
        
        material.roughness = sqrt((rand() % 1000) / 1000.0);
        

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
