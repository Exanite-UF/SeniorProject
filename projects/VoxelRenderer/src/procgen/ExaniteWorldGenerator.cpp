#include "ExaniteWorldGenerator.h"

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <src/utilities/Log.h>
#include <src/world/MaterialManager.h>

ExaniteWorldGenerator::ExaniteWorldGenerator(glm::ivec3 worldSize)
    : WorldGenerator(worldSize)
{
}

void ExaniteWorldGenerator::generateData()
{
    auto& materialManager = MaterialManager::getInstance();
    std::shared_ptr<Material> material;
    if (!materialManager.tryGetMaterialById(materialKey, material))
    {
        material = materialManager.getMaterialByIndex(0);
        Log::log("Failed to find material with id '" + materialKey + "'. Using default material '" + material->getKey() + "' instead.");
    }

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            for (int z = 0; z < data.getSize().y; ++z)
            {
                data.setVoxelMaterial(glm::ivec3(x, y, z), material);
            }
        }
    }

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            data.setVoxelOccupancy(glm::ivec3(x, y, 0), true);
        }
    }
}

void ExaniteWorldGenerator::showDebugMenu()
{
    if (ImGui::CollapsingHeader("Exanite's Generator (F6)"))
    {
        ImGui::InputText("Material", &materialKey);
    }
}
