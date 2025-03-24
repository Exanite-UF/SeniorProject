#include "ExampleWorldGenerator.h"

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>
#include <src/utilities/Log.h>
#include <src/world/MaterialManager.h>

void ExampleWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& materialManager = MaterialManager::getInstance();
    std::shared_ptr<Material> material;
    if (!materialManager.tryGetMaterialByKey(materialKey, material))
    {
        material = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find material with id '" + materialKey + "'. Using default material '" + material->getKey() + "' instead.");
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

void ExampleWorldGenerator::showDebugMenu()
{
    ImGui::PushID("ExampleWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Example World Generator"))
        {
            ImGui::InputText("Material", &materialKey);
        }
    }
    ImGui::PopID();
}
