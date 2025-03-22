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

void ExampleWorldGenerator::showDebugMenu()
{
    // To Fix the Long title issue for headers
    std::string headerText = "Example World Generator";

    float availableWidth = ImGui::GetContentRegionAvail().x * 0.9f;
    float textWidth = ImGui::CalcTextSize(headerText.c_str()).x;

    if (textWidth > availableWidth)
    {
        std::string ellipsis = "...";
        float ellipsisWidth = ImGui::CalcTextSize(ellipsis.c_str()).x;

        while (ImGui::CalcTextSize((headerText + ellipsis).c_str()).x > (availableWidth) && headerText.length() > 1)
        {
            headerText.pop_back();
        }

        headerText += ellipsis;
    }

    ImGui::PushID("ExampleWorldGenerator");
    {
        ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
        float indentSize = ImGui::GetWindowContentRegionMax().x / 16.0f;

        if (ImGui::CollapsingHeader(headerText.c_str()))
        {
            ImGui::Text("Material");
            ImGui::Indent(indentSize);
            ImGui::PushID("MaterialKey");
            ImGui::InputText("", &materialKey);
            ImGui::PopID();
            ImGui::Unindent(indentSize);
        }
        ImGui::PopTextWrapPos();
    }
    ImGui::PopID();
}
